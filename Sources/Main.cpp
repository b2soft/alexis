#include "Precompiled.h"

// The number of back buffers
const uint8_t g_numFrames = 2;

// Use WARP adapter
bool g_useWarp = false;

uint32_t g_clientWidth = 1280;
uint32_t g_clientHeight = 720;

// Set to true once the DX12 objects have been initialized.
bool g_isInitialized = false;


// Window handle
HWND g_hWnd;

// Window Rectangle (for fullscreen state)
RECT g_windowRect;

// DirectX 12 Objects
ComPtr<ID3D12Device2> g_device;
ComPtr<ID3D12CommandQueue> g_commandQueue;
ComPtr<IDXGISwapChain4> g_swapChain;
ComPtr<ID3D12Resource> g_backBuffers[g_numFrames];
ComPtr<ID3D12GraphicsCommandList> g_commandList;
ComPtr<ID3D12CommandAllocator> g_commandAllocators[g_numFrames];
ComPtr<ID3D12DescriptorHeap> g_rtvDescriptorHeap;
UINT g_rtvDescriptorSize;
UINT g_currentBackBufferIndex;

// Synchronization objects
ComPtr<ID3D12Fence> g_fence;
uint64_t g_fenceValue = 0;
uint64_t g_frameFenceValues[g_numFrames] = {};
HANDLE g_fenceEvent;

// Enable V-Sync by default
bool g_vSync = true;
bool g_tearingSupport = false;

// Windowed mode by default
bool g_fullscreen = false;

// Window callback function
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void ParseCommandLineArgs()
{
	int argc = 0;
	wchar_t** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
	for (size_t i = 0; i < argc; ++i)
	{
		if (::wcscmp(argv[i], L"-w") == 0 || ::wcscmp(argv[i], L"--width") == 0)
		{
			g_clientWidth = ::wcstol(argv[++i], nullptr, 10);
		}
		if (::wcscmp(argv[i], L"-h") == 0 || ::wcscmp(argv[i], L"--height") == 0)
		{
			g_clientHeight = ::wcstol(argv[++i], nullptr, 10);
		}
		if (::wcscmp(argv[i], L"-warp") == 0 || ::wcscmp(argv[i], L"--warp") == 0)
		{
			g_useWarp = true;
		}
	}

	// Free memory allocated by CommandLineToArgvW
	::LocalFree(argv);
}

// Before doing anything using either the DXGI or the Direct3D API, the debug layer should be enabled in debug builds.
// Enabling the debug layer after creating the ID3D12Device will cause the runtime to remove the device.
void EnableDebugLayer()
{
#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif
}

void RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName)
{
	// Register a window class for rendering
	WNDCLASSEX windowClass = {};

	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = &WndProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = hInst;
	windowClass.hIcon = ::LoadIcon(hInst, NULL);
	windowClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = windowClassName;
	windowClass.hIconSm = ::LoadIcon(hInst, NULL);

	static ATOM atom = ::RegisterClassExW(&windowClass);
	assert(atom > 0);
}

HWND CreateWindow(const wchar_t* windowClassName, HINSTANCE hInst, const wchar_t* windowTitle, uint32_t width, uint32_t height)
{
	int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

	RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
	::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	int windowWidth = windowRect.right - windowRect.left;
	int windowHeight = windowRect.bottom - windowRect.top;

	// Center the window within the screen. Clamp to 0, 0 for the top-left corner.
	int windowX = std::max<int>(0, (screenWidth - windowWidth) / 2);
	int windowY = std::max<int>(0, (screenHeight - windowHeight) / 2);

	HWND hWnd = ::CreateWindowEx(NULL, windowClassName, windowTitle, WS_OVERLAPPEDWINDOW, windowX, windowY, windowWidth, windowHeight, NULL, NULL, hInst, nullptr);
	assert(hWnd && "Failed to create window");

	return hWnd;
}

ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
{
	ComPtr<ID3D12CommandQueue> d3d12CommandQueue;

	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));

	return d3d12CommandQueue;
}

ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd, ComPtr<ID3D12CommandQueue> commandQueue, uint32_t width, uint32_t height, uint32_t bufferCount)
{
	ComPtr<IDXGISwapChain4> dxgiSwapChain4;
	ComPtr<IDXGIFactory4> dxgiFactory4;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = bufferCount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	// It is recommended to always allow tearing if tearing support is available.
	swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(commandQueue.Get(), hWnd, &swapChainDesc, nullptr, nullptr, &swapChain1));

	// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
   // will be handled manually.
	ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain1.As(&dxgiSwapChain4));

	return dxgiSwapChain4;
}

ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
	ComPtr<ID3D12DescriptorHeap> descriptorHeap;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = numDescriptors;
	desc.Type = type;

	ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

	return descriptorHeap;
}

void UpdateRenderTargetViews(ComPtr<ID3D12Device2> device, ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap)
{
	auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < g_numFrames; ++i)
	{
		ComPtr<ID3D12Resource> backBuffer;
		ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

		g_backBuffers[i] = backBuffer;

		rtvHandle.Offset(rtvDescriptorSize);
	}
}

ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
{
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

	return commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type)
{
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ThrowIfFailed(device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

	ThrowIfFailed(commandList->Close());

	return commandList;
}

ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> device)
{
	ComPtr<ID3D12Fence> fence;

	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

	return fence;
}

HANDLE CreateEventHandle()
{
	HANDLE fenceEvent;

	fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent && "Failed to create fence event");

	return fenceEvent;
}

uint64_t Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue)
{
	uint64_t fenceValueForSignal = ++fenceValue;
	ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValueForSignal));

	return fenceValueForSignal;
}

void WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration = std::chrono::milliseconds::max())
{
	if (fence->GetCompletedValue() < fenceValue)
	{
		ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
		::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
	}
}

void Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue, HANDLE fenceEvent)
{
	uint64_t fenceValueForSignal = Signal(commandQueue, fence, fenceValue);
	WaitForFenceValue(fence, fenceValueForSignal, fenceEvent);
}

void Update()
{
	static uint64_t frameCounter = 0;
	static double elapsedSeconds = 0.0;
	static std::chrono::high_resolution_clock clock;
	static auto t0 = clock.now();

	frameCounter++;
	auto t1 = clock.now();
	auto deltaTime = t1 - t0;
	t0 = t1;

	elapsedSeconds += deltaTime.count() * 1e-9;
	if (elapsedSeconds > 1.0)
	{
		char buffer[500];
		auto fps = frameCounter / elapsedSeconds;
		sprintf_s(buffer, 500, "FPS: %f\n", fps);
		OutputDebugStringA(buffer);

		frameCounter = 0;
		elapsedSeconds = 0.0;
	}
}

void Render()
{
	auto commandAllocator = g_commandAllocators[g_currentBackBufferIndex];
	auto backBuffer = g_backBuffers[g_currentBackBufferIndex];

	commandAllocator->Reset();
	g_commandList->Reset(commandAllocator.Get(), nullptr);

	// Clear the render target
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		g_commandList->ResourceBarrier(1, &barrier);

		FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(g_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), g_currentBackBufferIndex, g_rtvDescriptorSize);

		g_commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
	}

	// Present
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		g_commandList->ResourceBarrier(1, &barrier);
	}

	ThrowIfFailed(g_commandList->Close());

	ID3D12CommandList* const commandLists[] = { g_commandList.Get() };
	g_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	UINT syncInterval = g_vSync ? 1 : 0;
	UINT presentFlags = g_tearingSupport && !g_vSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	ThrowIfFailed(g_swapChain->Present(syncInterval, presentFlags));

	g_frameFenceValues[g_currentBackBufferIndex] = Signal(g_commandQueue, g_fence, g_fenceValue);

	g_currentBackBufferIndex = g_swapChain->GetCurrentBackBufferIndex();

	WaitForFenceValue(g_fence, g_frameFenceValues[g_currentBackBufferIndex], g_fenceEvent);
}

void Resize(uint32_t width, uint32_t height)
{
	if (g_clientWidth != width || g_clientHeight != height)
	{
		// Don't allow 0 size
		g_clientWidth = std::max(1u, width);
		g_clientHeight = std::max(1u, height);

		Flush(g_commandQueue, g_fence, g_fenceValue, g_fenceEvent);

		for (int i = 0; i < g_numFrames; ++i)
		{
			// Any references to the back buffers must be released before the swap chain can be resized
			g_backBuffers[i].Reset();
			g_frameFenceValues[i] = g_frameFenceValues[g_currentBackBufferIndex];
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		ThrowIfFailed(g_swapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(g_swapChain->ResizeBuffers(g_numFrames, g_clientWidth, g_clientHeight, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

		g_currentBackBufferIndex = g_swapChain->GetCurrentBackBufferIndex();

		UpdateRenderTargetViews(g_device, g_swapChain, g_rtvDescriptorHeap);
	}
}

void SetFullscreen(bool fullscreen)
{
	if (g_fullscreen != fullscreen)
	{
		g_fullscreen = fullscreen;

		if (g_fullscreen)
		{
			// Store the current window dimensions so they can be restored 
			// when switching out of fullscreen state.
			::GetWindowRect(g_hWnd, &g_windowRect);

			//Set window to be borderless
			UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

			::SetWindowLongW(g_hWnd, GWL_STYLE, windowStyle);

			// Query the name of the nearest display device for the window.
			// This is required to set the fullscreen dimensions of the window
			// when using a multi-monitor setup.
			HMONITOR hMonitor = ::MonitorFromWindow(g_hWnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFOEX monitorInfo = {};
			monitorInfo.cbSize = sizeof(MONITORINFOEX);
			::GetMonitorInfo(hMonitor, &monitorInfo);

			::SetWindowPos(g_hWnd, HWND_TOP,
				monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.top,
				monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE);

			::ShowWindow(g_hWnd, SW_MAXIMIZE);
		}
		else
		{
			// Restore all window decorators
			::SetWindowLong(g_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

			::SetWindowPos(g_hWnd, HWND_NOTOPMOST,
				g_windowRect.left,
				g_windowRect.top,
				g_windowRect.right - g_windowRect.left,
				g_windowRect.bottom - g_windowRect.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE);

			::ShowWindow(g_hWnd, SW_NORMAL);
		}
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (g_isInitialized)
	{
		switch (message)
		{
		case WM_PAINT:
			Update();
			Render();
			break;
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			bool alt = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

			switch (wParam)
			{
			case 'V':
				g_vSync = !g_vSync;
				break;
			case VK_ESCAPE:
				::PostQuitMessage(0);
				break;

			case VK_RETURN:
				if (alt)
				{
			case VK_F11:
				SetFullscreen(!g_fullscreen);
				}
			}
		}
		break;
		case WM_SYSCHAR:
			break;
		case WM_SIZE:
		{
			RECT clientRect = {};
			::GetClientRect(g_hWnd, &clientRect);

			int width = clientRect.right - clientRect.left;
			int height = clientRect.bottom - clientRect.top;

			Resize(width, height);
		}
		break;
		case WM_DESTROY:
			::PostQuitMessage(0);
			break;
		default:
			return ::DefWindowProcW(hWnd, message, wParam, lParam);
		}
	}

	return ::DefWindowProcW(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nShowCmd)
{
	// Windows 10 Creators update adds Per Monitor V2 DPI awareness context.
	// Using this awareness context allows the client area of the window 
	// to achieve 100% scaling while still allowing non-client window content to 
	// be rendered in a DPI sensitive fashion.
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	// Window class name. Used for registering / creating the window
	const wchar_t* windowClassName = L"b2nder Window";
	ParseCommandLineArgs();

	EnableDebugLayer();

	g_tearingSupport = CheckTearingSupport();

	RegisterWindowClass(hInstance, windowClassName);
	g_hWnd = CreateWindow(windowClassName, hInstance, L"b2nder Sample", g_clientWidth, g_clientHeight);

	// Initialize the global window rect variable
	::GetWindowRect(g_hWnd, &g_windowRect);

	// DirectX 12 Init
	ComPtr<IDXGIAdapter4> dxgiAdapter4 = GetAdapter(g_useWarp);

	g_device = CreateDevice(dxgiAdapter4);

	g_commandQueue = CreateCommandQueue(g_device, D3D12_COMMAND_LIST_TYPE_DIRECT);

	g_swapChain = CreateSwapChain(g_hWnd, g_commandQueue, g_clientWidth, g_clientHeight, g_numFrames);

	g_currentBackBufferIndex = g_swapChain->GetCurrentBackBufferIndex();

	g_rtvDescriptorHeap = CreateDescriptorHeap(g_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, g_numFrames);
	g_rtvDescriptorSize = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	UpdateRenderTargetViews(g_device, g_swapChain, g_rtvDescriptorHeap);

	for (int i = 0; i < g_numFrames; ++i)
	{
		g_commandAllocators[i] = CreateCommandAllocator(g_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	}
	g_commandList = CreateCommandList(g_device, g_commandAllocators[g_currentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);

	g_fence = CreateFence(g_device);
	g_fenceEvent = CreateEventHandle();

	g_isInitialized = true;

	::ShowWindow(g_hWnd, SW_SHOW);

	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	Flush(g_commandQueue, g_fence, g_fenceValue, g_fenceEvent);

	::CloseHandle(g_fenceEvent);

	return 0;
}