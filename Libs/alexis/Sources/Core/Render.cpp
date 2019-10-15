#include <Precompiled.h>

#include "Render.h"

#include "Core.h"

namespace alexis
{


	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT2 uv;
	};

	const UINT Render::k_frameCount;

	void Render::Initialize(int width, int height)
	{
		m_windowWidth = width;
		m_windowHeight = height;

		m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
		m_scissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(width), static_cast<LONG>(height));

		InitDevice();
		InitPipeline();
	}

	void Render::Destroy()
	{
		WaitForGpu();

		CloseHandle(m_fenceEvent);
	}

	void Render::BeginRender()
	{
		// Can be reset only if GPU has finished execution using such allocator
		ThrowIfFailed(m_clientCommandAllocators[m_frameIndex]->Reset());
		ThrowIfFailed(m_clientCommandList->Reset(m_clientCommandAllocators[m_frameIndex].Get(), nullptr));
	}

	void Render::Present()
	{
		// TODO: GUI here?

		UINT syncInterval = m_vSync ? 1 : 0;
		UINT presetFlags = m_isTearingSupported && !m_vSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
		ThrowIfFailed(m_swapChain->Present(syncInterval, presetFlags));

		MoveToNextFrame();
	}

	void Render::OnResize(int width, int height)
	{
		m_windowWidth = width;
		m_windowHeight = height;
	}

	void Render::ExecuteCommandList(ComPtr<ID3D12GraphicsCommandList> list)
	{
		list->Close();

		ID3D12CommandList* ppCommandLists[] = { list.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		//list->Reset(m_clientCommandAllocators[m_frameIndex].Get(), nullptr);
	}

	ComPtr<ID3D12GraphicsCommandList> Render::GetGraphicsCommandList()
	{
		return m_clientCommandList;
	}

	ID3D12Resource* Render::GetCurrentBackBufferResource() const
	{
		return m_backbufferTextures[m_frameIndex].Get();
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE Render::GetCurrentBackBufferRTV() const
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	}

	void Render::MoveToNextFrame()
	{
		// Schedule a signal in the queue
		const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];
		ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));

		// Update the frame index
		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

		// If the next frame is not ready, wait until it is ready
		if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
		{
			ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
			WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
		}

		// Set the fence value for the next frame
		m_fenceValues[m_frameIndex] = currentFenceValue + 1;
	}

	void Render::WaitForGpu()
	{
		// Schedule a signal in the queue
		ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]));

		// Wait until fence has been processed
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);

		// Increment the value for current frame
		m_fenceValues[m_frameIndex]++;
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

		HWND hwnd = Core::s_hwnd;

		// Create Swap Chain
		{
			DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
			swapChainDesc.BufferCount = k_frameCount;
			swapChainDesc.Width = m_windowWidth;
			swapChainDesc.Height = m_windowHeight;
			swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapChainDesc.SampleDesc.Count = 1;

			// Create Command Queue
			{
				D3D12_COMMAND_QUEUE_DESC queueDesc = {};
				queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
				queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

				ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
			}

			ComPtr<IDXGISwapChain1> swapChain;
			ThrowIfFailed(factory->CreateSwapChainForHwnd(
				m_commandQueue.Get(),
				hwnd,
				&swapChainDesc,
				nullptr,
				nullptr,
				&swapChain));

			ThrowIfFailed(swapChain.As(&m_swapChain));
		}

		ThrowIfFailed(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
	}

	void Render::InitPipeline()
	{
		// Create RTV Descriptors Heap for backbuffers
		{
			// Create RTV Descriptors Heap for backbuffers
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
			rtvHeapDesc.NumDescriptors = k_frameCount;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

			m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}

		// Create Frame Resources
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

			// Create RTV + alloc per back buffer
			for (UINT i = 0; i < k_frameCount; ++i)
			{
				ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backbufferTextures[i])));
				m_device->CreateRenderTargetView(m_backbufferTextures[i].Get(), nullptr, rtvHandle);
				rtvHandle.Offset(1, m_rtvDescriptorSize);

				// Create client Command Allocator
				ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_clientCommandAllocators[i])));
			}
		}

		// Create Client List
		ThrowIfFailed(m_device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_clientCommandAllocators[m_frameIndex].Get(),
			nullptr,
			IID_PPV_ARGS(&m_clientCommandList)));

		ThrowIfFailed(m_clientCommandList->Close());

		// Create Synchronization Objects
		{
			ThrowIfFailed(m_device->CreateFence(m_fenceValues[m_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
			m_fenceValues[m_frameIndex]++;

			m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			if (m_fenceEvent == nullptr)
			{
				ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
			}

			// Wait for the command list to execute; we are reusing the same command 
			// list in our main loop but for now, we just want to wait for setup to 
			// complete before continuing
			WaitForGpu();
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

}
