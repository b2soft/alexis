#include <Precompiled.h>

#include "Render.h"

#include "Core.h"
#include "CommandManager.h"

namespace alexis
{
	const UINT Render::k_frameCount;

	void Render::Initialize(int width, int height)
	{
		m_windowWidth = width;
		m_windowHeight = height;

		m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
		m_scissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(width), static_cast<LONG>(height));

		InitDevice();

		InitPipeline();

		for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			m_descriptorAllocators[i] = std::make_unique<DescriptorAllocator>(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
		}

		m_uploadBufferManager = std::make_unique<UploadBufferManager>();

		UpdateRenderTargetViews();
	}

	void Render::Destroy()
	{
		CommandManager::GetInstance()->WaitForGpu();

		CommandManager::GetInstance()->Destroy();
	}

	void Render::BeginRender()
	{
		CommandManager::GetInstance()->Release();

		WaitForSingleObjectEx(m_swapChainEvent, 100, FALSE);
	}

	void Render::Present()
	{
		UINT syncInterval = m_vSync ? 1 : 0;
		UINT presetFlags = m_isTearingSupported && !m_vSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
		ThrowIfFailed(m_swapChain->Present(syncInterval, presetFlags));

		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

		ReleaseStaleDescriptors(CommandManager::GetInstance()->GetLastCompletedFence());
	}

	void Render::OnResize(int width, int height)
	{
		if (m_windowWidth != width ||
			m_windowHeight != height)
		{
			m_windowWidth = std::max(1, width);
			m_windowHeight = std::max(1, height);

			CommandManager::GetInstance()->WaitForGpu();

			m_backbufferRT.AttachTexture(TextureBuffer(), RenderTarget::Slot0);
			for (int i = 0; i < k_frameCount; ++i)
			{
				m_backbufferTextures[i].Reset();
			}

			DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
			ThrowIfFailed(m_swapChain->GetDesc(&swapChainDesc));
			ThrowIfFailed(m_swapChain->ResizeBuffers(k_frameCount, m_windowWidth, m_windowHeight, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

			m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

			UpdateRenderTargetViews();
		}

	}

	const RenderTarget& Render::GetBackbufferRT() const
	{
		m_backbufferRT.AttachTexture(m_backbufferTextures[m_frameIndex], RenderTarget::Slot::Slot0);
		return m_backbufferRT;
	}

	const CD3DX12_VIEWPORT& Render::GetDefaultViewport() const
	{
		return m_viewport;
	}

	const CD3DX12_RECT& Render::GetDefaultScrissorRect() const
	{
		return m_scissorRect;
	}

	ID3D12Device2* Render::GetDevice() const
	{
		return m_device.Get();
	}

	alexis::UploadBufferManager* Render::GetUploadBufferManager() const
	{
		return m_uploadBufferManager.get();
	}

	bool Render::IsVSync() const
	{
		return m_vSync;
	}

	void Render::SetVSync(bool vSync)
	{
		m_vSync = vSync;
	}

	void Render::ToggleVSync()
	{
		SetVSync(!m_vSync);
	}

	bool Render::IsFullscreen() const
	{
		return m_fullscreen;
	}

	void Render::SetFullscreen(bool fullscreen)
	{
		if (m_fullscreen != fullscreen)
		{
			m_fullscreen = fullscreen;

			if (m_fullscreen)
			{
				GetWindowRect(Core::s_hwnd, &m_windowRect);

				UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

				SetWindowLongW(Core::s_hwnd, GWL_STYLE, windowStyle);

				// Query the name of the nearest display device for the window
				// This is required to set the fullscreen dimensions of the window when using a multi-monitor setup
				HMONITOR hMonitor = ::MonitorFromWindow(Core::s_hwnd, MONITOR_DEFAULTTONEAREST);
				MONITORINFOEX monitorInfo = {};
				monitorInfo.cbSize = sizeof(MONITORINFOEX);
				GetMonitorInfo(hMonitor, &monitorInfo);

				SetWindowPos(Core::s_hwnd, HWND_TOP,
					monitorInfo.rcMonitor.left,
					monitorInfo.rcMonitor.top,
					monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
					monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
					SWP_FRAMECHANGED | SWP_NOACTIVATE);

				ShowWindow(Core::s_hwnd, SW_MAXIMIZE);
			}
			else
			{
				// Restore all the window decorators
				SetWindowLong(Core::s_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

				SetWindowPos(Core::s_hwnd, HWND_NOTOPMOST,
					m_windowRect.left,
					m_windowRect.top,
					m_windowRect.right - m_windowRect.left,
					m_windowRect.bottom - m_windowRect.top,
					SWP_FRAMECHANGED | SWP_NOACTIVATE);

				ShowWindow(Core::s_hwnd, SW_NORMAL);
			}
		}
	}

	void Render::ToggleFullscreen()
	{
		SetFullscreen(!m_fullscreen);
	}

	DescriptorAllocation Render::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors /*= 1*/)
	{
		return m_descriptorAllocators[type]->Allocate(numDescriptors);
	}

	void Render::InitDevice()
	{
		UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
		// Enable debug layer
		// Note, that enabling the debug layer after device creation will invalidate the active device
		{
			ComPtr<ID3D12Debug1> debugInterface;
			ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));

			debugInterface->EnableDebugLayer();
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

			//debugInterface->SetEnableGPUBasedValidation(TRUE);
			//debugInterface->SetEnableSynchronizedCommandQueueValidation(TRUE);
		}
#endif

		ComPtr<IDXGIFactory4> factory;
		ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

		if (m_useWarpDevice)
		{
			ComPtr<IDXGIAdapter> warpAdapter;
			ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

			ThrowIfFailed(D3D12CreateDevice(
				warpAdapter.Get(),
				D3D_FEATURE_LEVEL_11_0,
				IID_PPV_ARGS(&m_device)));
		}
		else
		{
			ComPtr<IDXGIAdapter4> hardwareAdapter = GetHardwareAdapter(factory.Get());

			ThrowIfFailed(D3D12CreateDevice(
				hardwareAdapter.Get(),
				D3D_FEATURE_LEVEL_11_0,
				IID_PPV_ARGS(&m_device)));
		}

		// Enable debug messages in debug mode
#if defined(_DEBUG)
		ComPtr<ID3D12InfoQueue> infoQueue;
		if (SUCCEEDED(m_device.As(&infoQueue)))
		{
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
		}

		// Suppress whole message categories
		//D3D12_MESSAGE_CATEGORY suppressedCategories[] = {};

		// Suppress messages based on severity
		D3D12_MESSAGE_SEVERITY suppressedSeverities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress messages by specific ID
		D3D12_MESSAGE_ID suppressedIds[] =
		{
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
			D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_BEFORE_AFTER_MISMATCH
		};

		D3D12_INFO_QUEUE_FILTER newFilter = {};
		//newFilter.DenyList.NumCategories = _countof(suppressedCategories);
		//newFilter.DenyList.pCategoryList = suppressedCategories;
		newFilter.DenyList.NumSeverities = _countof(suppressedSeverities);
		newFilter.DenyList.pSeverityList = suppressedSeverities;
		newFilter.DenyList.NumIDs = _countof(suppressedIds);
		newFilter.DenyList.pIDList = suppressedIds;

		ThrowIfFailed(infoQueue->PushStorageFilter(&newFilter));
#endif

		// Init command manager to have command queue needed for swapchain
		CommandManager::GetInstance()->Initialize();


		// Check for G-Sync / FreeSync is available
		{
			BOOL allowTearing = FALSE;

			if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
			{
				ComPtr<IDXGIFactory5> factory5;
				if (SUCCEEDED(factory.As(&factory5)))
				{
					factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,
						&allowTearing, sizeof(allowTearing));
				}
			}

			m_isTearingSupported = (allowTearing == TRUE);
		}

		// Create Swap Chain

		HWND hwnd = Core::s_hwnd;

		{
			DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
			swapChainDesc.BufferCount = k_frameCount;
			swapChainDesc.Width = m_windowWidth;
			swapChainDesc.Height = m_windowHeight;
			swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapChainDesc.SampleDesc.Count = 1;
			swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
			swapChainDesc.Flags |= m_isTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

			ComPtr<IDXGISwapChain1> swapChain;
			ThrowIfFailed(factory->CreateSwapChainForHwnd(
				CommandManager::GetInstance()->m_directCommandQueue.Get(),
				hwnd,
				&swapChainDesc,
				nullptr,
				nullptr,
				&swapChain));

			ThrowIfFailed(swapChain.As(&m_swapChain));
		}

		m_swapChain->SetMaximumFrameLatency(k_frameCount);
		m_swapChainEvent = m_swapChain->GetFrameLatencyWaitableObject();

		ThrowIfFailed(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
	}

	void Render::InitPipeline()
	{

	}
	void Render::ReleaseStaleDescriptors(uint64_t fenceValue)
	{
		for (auto& allocator : m_descriptorAllocators)
		{
			allocator->ReleaseStaleDescriptors(fenceValue);
		}
	}

	Microsoft::WRL::ComPtr<IDXGIAdapter4> Render::GetHardwareAdapter(IDXGIFactory4* factory)
	{
		ComPtr<IDXGIFactory4> dxgiFactory;
		UINT createFactoryFlags = 0;
#if defined(_DEBUG)
		createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

		ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

		ComPtr<IDXGIAdapter1> dxgiAdapter1;
		ComPtr<IDXGIAdapter4> dxgiAdapter4;

		SIZE_T maxDedicatedVideoMemory = 0;
		for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

			if (dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see if the adapter can create a D3D12 device without actually creating it.
			// The adapter with the largest dedicated video memory is favored.
			if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
				SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
				dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
			{
				maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
				ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
			}
		}

		return dxgiAdapter4;
	}

	void Render::UpdateRenderTargetViews()
	{
		for (int i = 0; i < k_frameCount; ++i)
		{
			ComPtr<ID3D12Resource> backBuffer;
			ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

			m_backbufferTextures[i].SetFromResource(backBuffer.Get());
		}
	}
}
