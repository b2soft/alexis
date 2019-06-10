#include "Precompiled.h"

bool g_isRunning = true;

// WinAPI Stuff (temporary, will be refactored)

HWND g_hwnd = nullptr;

const LPCTSTR k_windowName = L"Render DX12";
const LPCTSTR k_windowTitle = L"Window";

const int k_width = 800;
const int k_height = 600;

const bool k_fullScreen = false;

// DirectX Stuff (temporary, will be refactored)

const int frameBufferCount = 3; // Triple Buffering
ID3D12Device* device;
IDXGISwapChain3* swapChain; // Defines way to switch between RT
ID3D12CommandQueue* commandQueue; // Container for command lists
ID3D12DescriptorHeap* rtvDescriptorHeap; // Descriptor heap to hold like info about RT
ID3D12Resource* renderTargets[frameBufferCount]; // RTs
ID3D12CommandAllocator* commandAllocator[frameBufferCount]; // Allocators number is frameBuffers * number of threads (1 thread for now)
ID3D12GraphicsCommandList* commandList; // Command list
ID3D12Fence* fence[frameBufferCount]; // Object to be locked while command list is executed on GPU. Need as many, as allocators number
HANDLE fenceEvent; // Event for Fence object unlocking
UINT64 fenceValue[frameBufferCount]; // Incremented each frame, each fence has its own value
int frameIndex; // Current RTV rendered
int rtvDescriptorSize; // Size of the RTV descriptor on the device

bool InitD3D()
{
	HRESULT hr;

	// Create the Device

	IDXGIFactory4* dxgiFactory;
	hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(hr))
	{
		return false;
	}

	IDXGIAdapter1* adapter; // Graphics cards
	int adapterIndex = 0; // Start from device 0
	bool adapterFound = false; // Set to true if we find DX12 device

	// Find first hardware GPU with DX12
	while (dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// Go to next adapter, software is not applicable
			adapterIndex++;
			continue;
		}

		// Check for DX 12 compatibility but test-create the fake device
		hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(hr))
		{
			adapterFound = true;
			break;
		}

		adapterIndex++;
	}

	if (!adapterFound)
	{
		return false;
	}

	// Create the device
	hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
	if (FAILED(hr))
	{
		return false;
	}

	// Create the Command Queue
	D3D12_COMMAND_QUEUE_DESC cqDesc = {};
	hr = device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&commandQueue));
	if (FAILED(hr))
	{
		return false;
	}

	// Swap Chain
	DXGI_MODE_DESC backBufferDesc = {};
	backBufferDesc.Width = k_width;
	backBufferDesc.Height = k_height;
	backBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Format of the back buffer

	// Multi-sampling (disabled)
	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count = 1;

	// Describe and create Swap Chain
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = frameBufferCount;
	swapChainDesc.BufferDesc = backBufferDesc;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = g_hwnd;
	swapChainDesc.SampleDesc = sampleDesc;
	swapChainDesc.Windowed = !k_fullScreen;

	IDXGISwapChain* tempSwapChain;

	dxgiFactory->CreateSwapChain(commandQueue, &swapChainDesc, &tempSwapChain);

	swapChain = static_cast<IDXGISwapChain3*>(tempSwapChain);

	frameIndex = swapChain->GetCurrentBackBufferIndex();

	// Create RTV (backbuffers) Descriptor Heap

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = frameBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	// Not visible to shades directly
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));
	if (FAILED(hr))
	{
		return false;
	}

	// Size of desc in the RTV heap. Needed to increment offsets
	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Get a desc handle in desc heap. Handle is like a pointer but not C++ pointer
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// Create RTV for each buffer
	for (int i = 0; i < frameBufferCount; i++)
	{
		// Get n'th buffer in the swap chain and store it in the n'th position of the ID3D12Resource array
		hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
		if (FAILED(hr))
		{
			return false;
		}
		// "Create" a RTV that binds buffer to RTV handle
		device->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);

		rtvHandle.Offset(1, rtvDescriptorSize);
	}

	// Create Command Allocators
	for (int i = 0; i < frameBufferCount; i++)
	{
		hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator[i]));
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Create Command Lists
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator[0], nullptr, IID_PPV_ARGS(&commandList));
	if (FAILED(hr))
	{
		return false;
	}

	// Default state of command list is Writing, so close it and write later
	commandList->Close();

	// Create Fence for each buffer
	for (int i = 0; i < frameBufferCount; i++)
	{
		hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence[i]));
		if (FAILED(hr))
		{
			return false;
		}
		fenceValue[i] = 0;
	}

	// Create a handle to a fence event
	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fenceEvent == nullptr)
	{
		return false;
	}

	return true;
}

void Update() // Update logic
{
	// Game logic will be here
}

void WaitForPreviousFrame() // Wait until GPU is finished with command list
{
	HRESULT hr;

	// Swap the current RTV buffer and draw to the correct buffer
	frameIndex = swapChain->GetCurrentBackBufferIndex();

	// If the fence value is less than fenceValue, GPU is not finished execution

	if (fence[frameIndex]->GetCompletedValue() < fenceValue[frameIndex])
	{
		hr = fence[frameIndex]->SetEventOnCompletion(fenceValue[frameIndex], fenceEvent);
		if (FAILED(hr))
		{
			g_isRunning = false;
		}

		WaitForSingleObject(fenceEvent, INFINITE);
	}

	fenceValue[frameIndex]++;
}


void UpdatePipeline() // Update D3D (command lists)
{
	HRESULT hr;

	// Wait for previous command allocator to finish
	WaitForPreviousFrame();

	hr = commandAllocator[frameIndex]->Reset();
	if (FAILED(hr))
	{
		g_isRunning = false;
	}

	// Reset command list. Resetting puts list into a recording state. Here initial pipeline state object is passed
	hr = commandList->Reset(commandAllocator[frameIndex], nullptr);
	if (FAILED(hr))
	{
		g_isRunning = false;
	}


	// Put commands into command list

	// Transition from present state -> RT state to render there
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Handle to current RTV
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);

	// Set Render Target for OM
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Clear the RT
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// Transition from RT -> present state
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	hr = commandList->Close();
	if (FAILED(hr))
	{
		g_isRunning = false;
	}
}

void Render() // Execute command lists
{
	HRESULT hr;

	UpdatePipeline();

	// Execute array of command lists
	ID3D12CommandList* ppCommandLists[] = { commandList };

	// Execute the array of command lists
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// This signal goes at the end of command queue
	hr = commandQueue->Signal(fence[frameIndex], fenceValue[frameIndex]);
	if (FAILED(hr))
	{
		g_isRunning = false;
	}

	// Present the current backbuffer
	hr = swapChain->Present(0, 0);
	if (FAILED(hr))
	{
		g_isRunning = false;
	}
}

void Cleanup() // release COM objects and clean up memory
{
	for (int i = 0; i < frameBufferCount; i++)
	{
		frameIndex = i;
		WaitForPreviousFrame();
	}

	BOOL fs = false;
	if (swapChain->GetFullscreenState(&fs, NULL))
	{
		swapChain->SetFullscreenState(FALSE, NULL);
	}

	SAFE_RELEASE(device);
	SAFE_RELEASE(swapChain);
	SAFE_RELEASE(commandQueue);
	SAFE_RELEASE(rtvDescriptorHeap);
	SAFE_RELEASE(commandList);

	for (int i = 0; i < frameBufferCount; i++)
	{
		SAFE_RELEASE(renderTargets[i]);
		SAFE_RELEASE(commandAllocator[i]);
		SAFE_RELEASE(fence[i]);
	}
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool InitializeWindow(HINSTANCE hInstance, int showWnd, int width, int height, bool fullscreen)
{
	if (fullscreen)
	{
		HMONITOR hMon = MonitorFromWindow(g_hwnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(hMon, &mi);

		width = mi.rcMonitor.right - mi.rcMonitor.left;
		height = mi.rcMonitor.bottom - mi.rcMonitor.top;
	}

	WNDCLASSEX wc;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = k_windowName;
	wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		MessageBox(0, L"Error registering WNDCLASSEX!", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	g_hwnd = CreateWindowEx(0, k_windowName, k_windowTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, k_width, k_height, nullptr, NULL, hInstance, nullptr);

	if (!g_hwnd)
	{
		MessageBox(NULL, L"Error creating window!", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	if (fullscreen)
	{
		SetWindowLong(g_hwnd, GWL_STYLE, 0);
	}

	ShowWindow(g_hwnd, showWnd);
	UpdateWindow(g_hwnd);

	return true;
}

void MainLoop()
{
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	while (g_isRunning)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// Update Game Logic
		}
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			if (MessageBox(0, L"Are you sure you want to exit?", L"Really?", MB_YESNO | MB_ICONQUESTION) == IDYES)
			{
				DestroyWindow(hWnd);
			}
		}
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nShowCmd)
{
	if (!InitializeWindow(hInstance, nShowCmd, k_width, k_height, k_fullScreen))
	{
		MessageBox(0, L"Window initialization failed!", L"Error", MB_OK);
		return 1;
	}

	if (!InitD3D())
	{
		MessageBox(0, L"Failed to initialize DirectX 12", L"Error", MB_OK);
		Cleanup();
		return 1;
	}

	MainLoop();

	WaitForPreviousFrame();

	CloseHandle(fenceEvent);

	Cleanup();

	return 0;
}