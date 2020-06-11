#include <Precompiled.h>

#include "Render.h"

#include <Core/Core.h>
#include <Render/CommandManager.h>

#define ENABLE_DEBUG_LAYER

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

		for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			m_descriptorAllocators[i] = std::make_unique<DescriptorAllocator>(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
		}

		m_uploadBufferManager = std::make_unique<UploadBufferManager>();
		m_rtManager = std::make_unique<RenderTargetManager>();

		m_frameRenderGraph = std::make_unique<FrameRenderGraph>();

		UpdateRenderTargetViews();

		InitPipeline();
	}

	void Render::Destroy()
	{
		m_commandManager->WaitForGpu();

		m_commandManager.reset();
	}

	void Render::BeginRender()
	{
		WaitForSingleObjectEx(m_swapChainEvent, 100, FALSE);

		m_frameRenderGraph->Render();
	}

	void Render::Present()
	{
		UINT syncInterval = m_vSync ? 1 : 0;
		UINT presetFlags = m_isTearingSupported && !m_vSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
		ThrowIfFailed(m_swapChain->Present(syncInterval, presetFlags));

		auto& queue = m_commandManager->GetGraphicsQueue();
		auto fv = queue.SignalFence();
		m_fenceValues[m_frameIndex] = fv;

		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

		queue.WaitForFence(m_fenceValues[m_frameIndex]);
	}

	void Render::OnResize(int width, int height)
	{
		if (m_windowWidth != width ||
			m_windowHeight != height)
		{
			m_windowWidth = std::max(1, width);
			m_windowHeight = std::max(1, height);

			m_commandManager->WaitForGpu();

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

	const CD3DX12_RECT& Render::GetDefaultScissorRect() const
	{
		return m_scissorRect;
	}

	ID3D12Device2* Render::GetDevice() const
	{
		return m_device.Get();
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

	Render::DescriptorRecord Render::AllocateSRV(ID3D12Resource* resource, D3D12_SHADER_RESOURCE_VIEW_DESC desc)
	{
		DescriptorRecord record;

		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_srvUavHeap->GetCPUDescriptorHandleForHeapStart(), m_allocatedSrvUavs, m_incrementSrvUav);

		m_device->CreateShaderResourceView(resource, &desc, handle);

		//record.CpuPtr = handle.ptr;
		//record.GpuPtr = handle.ptr;
		record.OffsetInHeap = m_allocatedSrvUavs;

		m_allocatedSrvUavs++;

		return record;
	}

	ID3D12DescriptorHeap* Render::GetSrvUavHeap()
	{
		return m_srvUavHeap.Get();
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
#if defined(ENABLE_DEBUG_LAYER)
		{
			ComPtr<ID3D12Debug1> debugInterface;
			ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));

			debugInterface->EnableDebugLayer();
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

			//debugInterface->SetEnableGPUBasedValidation(TRUE);
			//debugInterface->SetEnableSynchronizedCommandQueueValidation(TRUE);
		}
#endif

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

#if defined(ENABLE_DEBUG_LAYER)

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

#endif

		// Init command manager to have command queue needed for swapchain
		m_commandManager = std::make_unique<CommandManager>();

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
				m_commandManager->GetGraphicsQueue().GetCommandQueue(),
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
		m_incrementSrvUav = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_incrementRtv = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_incrementDsv = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		D3D12_DESCRIPTOR_HEAP_DESC srvUavDesc = {};
		srvUavDesc.NumDescriptors = 4096;
		srvUavDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvUavDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		m_device->CreateDescriptorHeap(&srvUavDesc, IID_PPV_ARGS(&m_srvUavHeap));

		D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
		rtvDesc.NumDescriptors = 4096;
		rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		m_device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&m_rtvHeap));

		D3D12_DESCRIPTOR_HEAP_DESC dsvDesc = {};
		dsvDesc.NumDescriptors = 4096;
		dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		m_device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&m_dsvHeap));


		// Depth
		//DXGI_FORMAT depthFormat = DXGI_FORMAT_D32_FLOAT;
		DXGI_FORMAT depthFormat = DXGI_FORMAT_R24G8_TYPELESS;
		auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthFormat, alexis::g_clientWidth, alexis::g_clientHeight);
		depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE depthClearValue;
		depthClearValue.Format = depthDesc.Format;
		depthClearValue.DepthStencil = { 1.0f, 0 };

		TextureBuffer depthTexture;
		depthTexture.Create(depthDesc, &depthClearValue);

		// Create a G-Buffer
		{
			auto gbuffer = std::make_unique<RenderTarget>(true);

			//DXGI_FORMAT gbufferColorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			DXGI_FORMAT gbufferColorFormat = DXGI_FORMAT_R16G16B16A16_UNORM;
			auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(gbufferColorFormat, alexis::g_clientWidth, alexis::g_clientHeight, 1, 1);
			colorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

			D3D12_CLEAR_VALUE colorClearValue;
			colorClearValue.Format = colorDesc.Format;
			colorClearValue.Color[0] = 0.0f;
			colorClearValue.Color[1] = 0.0f;
			colorClearValue.Color[2] = 0.0f;
			colorClearValue.Color[3] = 0.0f;

			TextureBuffer gb0;
			gb0.Create(colorDesc, &colorClearValue); // Base color
			gbuffer->AttachTexture(gb0, RenderTarget::Slot::Slot0);
			gb0.GetResource()->SetName(L"GB 0");

			TextureBuffer gb1;
			gb1.Create(colorDesc, &colorClearValue); // Normals
			gbuffer->AttachTexture(gb1, RenderTarget::Slot::Slot1);
			gb1.GetResource()->SetName(L"GB 1");

			TextureBuffer gb2;
			gb2.Create(colorDesc, &colorClearValue); // Base color
			gbuffer->AttachTexture(gb2, RenderTarget::Slot::Slot2);
			gb2.GetResource()->SetName(L"GB 2");

			gbuffer->AttachTexture(depthTexture, RenderTarget::Slot::DepthStencil);
			depthTexture.GetResource()->SetName(L"GB Depth");

			m_rtManager->EmplaceTarget(L"GB", std::move(gbuffer));
		}

		// Create an HDR RT
		{
			auto hdrTarget = std::make_unique<RenderTarget>(true);
			DXGI_FORMAT hdrFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

			auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(hdrFormat, alexis::g_clientWidth, alexis::g_clientHeight, 1, 1);
			colorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

			D3D12_CLEAR_VALUE colorClearValue;
			colorClearValue.Format = colorDesc.Format;
			colorClearValue.Color[0] = 0.0f;
			colorClearValue.Color[1] = 0.0f;
			colorClearValue.Color[2] = 0.0f;
			colorClearValue.Color[3] = 1.0f;

			TextureBuffer hdrTexture;
			hdrTexture.Create(colorDesc, &colorClearValue);
			hdrTexture.GetResource()->SetName(L"HDR Texture");

			hdrTarget->AttachTexture(hdrTexture, RenderTarget::Slot::Slot0);
			//hdrTarget->AttachTexture(depthTexture, RenderTarget::Slot::DepthStencil);

			m_rtManager->EmplaceTarget(L"HDR", std::move(hdrTarget));
		}

		// Create shadow map target
		{
			DXGI_FORMAT shadowFormat = DXGI_FORMAT_R24G8_TYPELESS;
			auto shadowDesc = CD3DX12_RESOURCE_DESC::Tex2D(shadowFormat, 1024, 1024);
			shadowDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

			D3D12_CLEAR_VALUE shadowDepthClearValue;
			shadowDepthClearValue.Format = shadowDesc.Format;
			shadowDepthClearValue.DepthStencil = { 1.0f, 0 };

			TextureBuffer shadowDepthTexture;
			shadowDepthTexture.Create(shadowDesc, &shadowDepthClearValue);

			auto shadowRT = std::make_unique<RenderTarget>();
			shadowRT->AttachTexture(shadowDepthTexture, RenderTarget::DepthStencil);
			m_rtManager->EmplaceTarget(L"Shadow Map", std::move(shadowRT));
		}

		// Wait
		m_commandManager->WaitForGpu();

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
#if defined(_DEBUG) && defined(ENABLE_DEBUG_LAYER)
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

