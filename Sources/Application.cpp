#include "Precompiled.h"

#include "Application.h"

#include "Game.h"
#include "CommandQueue.h"
#include "Window.h"

#include "Render/DescriptorAllocator.h"

static constexpr wchar_t k_windowClassName[] = L"b2nder Window Class";

using WindowPtr = std::shared_ptr<Window>;
using WindowMap = std::map<HWND, WindowPtr>;
using WindowNameMap = std::map<std::wstring, WindowPtr>;

static Application* s_singleton = nullptr;
static WindowMap s_windows;
static WindowNameMap s_windowsByName;

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

uint64_t Application::s_frameCount = 0;

// Wrapper to allow shared pointers for Window
struct MakeWindow : public Window
{
	MakeWindow(HWND hWnd, const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync)
		: Window(hWnd, windowName, clientWidth, clientHeight, vSync)
	{

	}
};

Application::Application(HINSTANCE hInst)
	: m_hInstance(hInst)
	, m_tearingSupported(false)
{
	// Windows 10 Creators update adds Per Monitor V2 DPI awareness context.
	// Using this awareness context allows the client area of the window 
	// to achieve 100% scaling while still allowing non-client window content to 
	// be rendered in a DPI sensitive fashion.
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	// Register a window class for rendering
	WNDCLASSEX windowClass = {};

	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = &WndProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = m_hInstance;
	windowClass.hIcon = ::LoadIcon(m_hInstance, NULL);
	windowClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = k_windowClassName;
	windowClass.hIconSm = ::LoadIcon(m_hInstance, NULL);

	if (!RegisterClassExW(&windowClass))
	{
		MessageBoxA(NULL, "Unable to register window class!", "Error", MB_OK | MB_ICONERROR);
	}
}

Application::~Application()
{
	Flush();
}

void Application::Initialize()
{
	// Before doing anything using either the DXGI or the Direct3D API, the debug layer should be enabled in debug builds.
// Enabling the debug layer after creating the ID3D12Device will cause the runtime to remove the device.
#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();

	// Enable these if you want full validation (will slow down rendering a lot)
	// debugInterface->SetEnableGPUBasedValidation(TRUE);
	// debugInterface->SetEnableSynchronizedCommandQueueValidation(TRUE);
#endif

	auto dxgiAdapter = GetAdapter(false);
	if (!dxgiAdapter)
	{
		// If no DX12 adapter exist - fall back to WARP
		dxgiAdapter = GetAdapter(true);
	}

	if (dxgiAdapter)
	{
		m_d3d12Device = CreateDevice(dxgiAdapter);
	}
	else
	{
		throw std::exception("DXGI adapter enumeration failed");
	}

	m_directCommandQueue = std::make_shared<CommandQueue>(D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_computeCommandQueue = std::make_shared<CommandQueue>(D3D12_COMMAND_LIST_TYPE_COMPUTE);
	m_copyCommandQueue = std::make_shared<CommandQueue>(D3D12_COMMAND_LIST_TYPE_COPY);

	m_tearingSupported = CheckTearingSupport();

	// Create descriptor allocators
	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_descriptorAllocators[i] = std::make_unique<DescriptorAllocator>(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
	}

	s_frameCount = 0;
}

Microsoft::WRL::ComPtr<IDXGIAdapter4> Application::GetAdapter(bool useWarp) const
{
	ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	ComPtr<IDXGIAdapter1> dxgiAdapter1;
	ComPtr<IDXGIAdapter4> dxgiAdapter4;

	if (useWarp)
	{
		ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
		ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
	}
	else
	{
		SIZE_T maxDedicatedVideoMemory = 0;
		for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

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
	}

	return dxgiAdapter4;
}

Microsoft::WRL::ComPtr<ID3D12Device2> Application::CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter)
{
	ComPtr<ID3D12Device2> d3d12Device2;
	ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));

	// Enable debug messages in debug mode
#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(d3d12Device2.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
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
		D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
		D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
	};

	D3D12_INFO_QUEUE_FILTER newFilter = {};
	//newFilter.DenyList.NumCategories = _countof(suppressedCategories);
	//newFilter.DenyList.pCategoryList = suppressedCategories;
	newFilter.DenyList.NumSeverities = _countof(suppressedSeverities);
	newFilter.DenyList.pSeverityList = suppressedSeverities;
	newFilter.DenyList.NumIDs = _countof(suppressedIds);
	newFilter.DenyList.pIDList = suppressedIds;

	ThrowIfFailed(pInfoQueue->PushStorageFilter(&newFilter));
#endif

	return d3d12Device2;
}

bool Application::CheckTearingSupport()
{
	BOOL allowTearing = FALSE;

	// Rather than create the DXGI 1.5 factory interface directly, we create the
	// DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
	// graphics debugging tools which will not support the 1.5 factory interface 
	// until a future update.

	ComPtr<IDXGIFactory4> factory4;
	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
	{
		ComPtr<IDXGIFactory5> factory5;
		if (SUCCEEDED(factory4.As(&factory5)))
		{
			if (FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
			{
				allowTearing = FALSE;
			}
		}
	}

	return allowTearing == TRUE;
}

void Application::Create(HINSTANCE hInst)
{
	if (!s_singleton)
	{
		s_singleton = new Application(hInst);
		s_singleton->Initialize();
	}
}

void Application::Destroy()
{
	if (s_singleton)
	{
		assert(s_windows.empty() && s_windowsByName.empty() && "All windows should be destroyed prior to destroy application instance");

		delete s_singleton;
		s_singleton = nullptr;
	}
}

Application& Application::Get()
{
	assert(s_singleton);
	return *s_singleton;
}

bool Application::IsTearingSupported() const
{
	return m_tearingSupported;
}

std::shared_ptr<Window> Application::CreateRenderWindow(const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync /*= true*/)
{
	// Check if window exists already
	auto windowIt = s_windowsByName.find(windowName);
	if (windowIt != s_windowsByName.end())
	{
		return windowIt->second;
	}

	RECT windowRect = { 0, 0, static_cast<LONG>(clientWidth), static_cast<LONG>(clientHeight) };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	HWND hWnd = ::CreateWindowW(k_windowClassName, windowName.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, // windowX windowY maybe?
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr, nullptr, m_hInstance, nullptr);

	if (!hWnd)
	{
		MessageBoxA(NULL, "Could not create the window for rendering", "Error", MB_OK | MB_ICONERROR);
		return nullptr;
	}

	WindowPtr window = std::make_shared<MakeWindow>(hWnd, windowName, clientWidth, clientHeight, vSync);
	window->Initialize();

	s_windows.insert(WindowMap::value_type(hWnd, window));
	s_windowsByName.insert(WindowNameMap::value_type(windowName, window));

	return window;
}

void Application::DestroyWindow(const std::wstring& windowName)
{
	WindowPtr window = GetWindowByName(windowName);
	if (window)
	{
		DestroyWindow(window);
	}
}

void Application::DestroyWindow(std::shared_ptr<Window> window)
{
	if (window)
	{
		window->Destroy();
	}
}

std::shared_ptr<Window> Application::GetWindowByName(const std::wstring& windowName)
{
	std::shared_ptr<Window> window;
	auto it = s_windowsByName.find(windowName);
	if (it != s_windowsByName.end())
	{
		window = it->second;
	}

	return window;
}

int Application::Run(std::shared_ptr<Game> game)
{
	if (!game->Initialize())
	{
		return 1;
	}

	if (!game->LoadContent())
	{
		return 2;
	}

	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// Flush any commands in the queues before quit
	Flush();

	game->UnloadContent();
	game->Destroy();

	return static_cast<int>(msg.wParam);
}

void Application::Quit(int returnCode /*= 0*/)
{
	PostQuitMessage(returnCode);
}

Microsoft::WRL::ComPtr<ID3D12Device2> Application::GetDevice() const
{
	return m_d3d12Device;
}

std::shared_ptr<CommandQueue> Application::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type /*= D3D12_COMMAND_LIST_TYPE_DIRECT*/) const
{
	std::shared_ptr<CommandQueue> commandQueue;
	switch (type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		commandQueue = m_directCommandQueue;
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		commandQueue = m_computeCommandQueue;
		break;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		commandQueue = m_copyCommandQueue;
		break;
	default:
		assert(false && "Invalid command queue type");
	}

	return commandQueue;
}

void Application::Flush()
{
	m_directCommandQueue->Flush();
	m_computeCommandQueue->Flush();
	m_copyCommandQueue->Flush();
}

DescriptorAllocation Application::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors /*= 1*/)
{
	return m_descriptorAllocators[type]->Allocate(numDescriptors);
}

void Application::ReleaseStaleDescriptors(uint64_t finishedFrame)
{
	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_descriptorAllocators[i]->ReleaseStaleDescriptors(finishedFrame);
	}
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Application::CreateDescriptorHeap(UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = type;
	desc.NumDescriptors = numDescriptors;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 0;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	ThrowIfFailed(m_d3d12Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

	return descriptorHeap;
}

UINT Application::GetDescriptorHeapIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
	return m_d3d12Device->GetDescriptorHandleIncrementSize(type);
}

// Remove window from lists
static void RemoveWindow(HWND hWnd)
{
	auto windowIt = s_windows.find(hWnd);
	if (windowIt != s_windows.end())
	{
		WindowPtr window = windowIt->second;
		s_windowsByName.erase(window->GetWindowName());
		s_windows.erase(windowIt);
	}
}

// Convert message ID into MouseButtonId
MouseButtonEventArgs::MouseButton DecodeMouseButton(UINT messageID)
{
	MouseButtonEventArgs::MouseButton mouseBtn = MouseButtonEventArgs::None;
	switch (messageID)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	{
		mouseBtn = MouseButtonEventArgs::Left;
	}
	break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	{
		mouseBtn = MouseButtonEventArgs::Right;
	}
	break;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	{
		mouseBtn = MouseButtonEventArgs::Middle;
	}
	break;
	}

	return mouseBtn;
}

static LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
	{
		return true;
	}

	WindowPtr window;
	{
		auto windowIt = s_windows.find(hWnd);
		if (windowIt != s_windows.end())
		{
			window = windowIt->second;
		}
	}

	if (window)
	{
		switch (message)
		{
		case WM_PAINT:
		{
			// DT is get from the Window
			UpdateEventArgs updateEventArgs(0.0f, 0.0f);
			window->OnUpdate(updateEventArgs);
			RenderEventArgs renderEventArgs(0.0f, 0.0f);
			window->OnRender(renderEventArgs);
		}
		break;

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			MSG charMsg;
			// Get the Unicode utf-16 char
			unsigned int c = 0;

			// For printable characters, the next message will be WM_CHAR.
			// This message contains the character code we need to send the KeyPressed event.
			// Inspired by the SDL 1.2 implementation.
			if (PeekMessage(&charMsg, hWnd, 0, 0, PM_NOREMOVE) && charMsg.message == WM_CHAR)
			{
				GetMessage(&charMsg, hWnd, 0, 0);
				c = static_cast<unsigned int>(charMsg.wParam);
			}

			bool shift = (GetAsyncKeyState(VK_SHIFT & 0x8000) != 0);
			bool control = (GetAsyncKeyState(VK_CONTROL & 0x8000) != 0);
			bool alt = (GetAsyncKeyState(VK_MENU & 0x8000) != 0);
			KeyCode::Key key = (KeyCode::Key)wParam;
			unsigned int scanCode = (lParam & 0x00FF0000) >> 16;
			KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Pressed, shift, control, alt);
			window->OnKeyPressed(keyEventArgs);
		}
		break;

		case WM_SYSKEYUP:
		case WM_KEYUP:
		{
			bool shift = (GetAsyncKeyState(VK_SHIFT & 0x8000) != 0);
			bool control = (GetAsyncKeyState(VK_CONTROL & 0x8000) != 0);
			bool alt = (GetAsyncKeyState(VK_MENU & 0x8000) != 0);
			KeyCode::Key key = (KeyCode::Key)wParam;
			unsigned int c = 0;
			unsigned int scanCode = (lParam & 0x00FF0000) >> 16;

			// Determine which key was released by converting the key code and the scan code
			// to a printable character (if possible).
			// Inspired by the SDL 1.2 implementation.
			unsigned char keyboardState[256];
			GetKeyboardState(keyboardState);
			wchar_t translatedCharacters[4];
			if (int result = ToUnicodeEx(static_cast<UINT>(wParam), scanCode, keyboardState, translatedCharacters, 4, 0, NULL) > 0)
			{
				c = translatedCharacters[0];
			}

			KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Released, shift, control, alt);
			window->OnKeyReleased(keyEventArgs);
		}
		break;

		// The default window procedure will play a system notification sound 
		// when pressing the Alt+Enter keyboard combination if this message is not handled
		case WM_SYSCHAR:
			break;

		case WM_MOUSEMOVE:
		{
			bool lButton = (wParam & MK_LBUTTON) != 0;
			bool rButton = (wParam & MK_RBUTTON) != 0;
			bool mButton = (wParam & MK_MBUTTON) != 0;
			bool shift = (wParam & MK_SHIFT) != 0;
			bool control = (wParam & MK_CONTROL) != 0;

			int x = ((int)(short)LOWORD(lParam));
			int y = ((int)(short)HIWORD(lParam));

			MouseMotionEventArgs mouseMotionEventArgs(lButton, mButton, rButton, control, shift, x, y);
			window->OnMouseMoved(mouseMotionEventArgs);
		}
		break;

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		{
			bool lButton = (wParam & MK_LBUTTON) != 0;
			bool rButton = (wParam & MK_RBUTTON) != 0;
			bool mButton = (wParam & MK_MBUTTON) != 0;
			bool shift = (wParam & MK_SHIFT) != 0;
			bool control = (wParam & MK_CONTROL) != 0;

			int x = ((int)(short)LOWORD(lParam));
			int y = ((int)(short)HIWORD(lParam));

			MouseButtonEventArgs mouseButtonEventArgs(DecodeMouseButton(message), MouseButtonEventArgs::Pressed, lButton, mButton, rButton, control, shift, x, y);
			window->OnMouseButtonPressed(mouseButtonEventArgs);
		}
		break;

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		{
			bool lButton = (wParam & MK_LBUTTON) != 0;
			bool rButton = (wParam & MK_RBUTTON) != 0;
			bool mButton = (wParam & MK_MBUTTON) != 0;
			bool shift = (wParam & MK_SHIFT) != 0;
			bool control = (wParam & MK_CONTROL) != 0;

			int x = ((int)(short)LOWORD(lParam));
			int y = ((int)(short)HIWORD(lParam));

			MouseButtonEventArgs mouseButtonEventArgs(DecodeMouseButton(message), MouseButtonEventArgs::Released, lButton, mButton, rButton, control, shift, x, y);
			window->OnMouseButtonReleased(mouseButtonEventArgs);
		}
		break;

		case WM_MOUSEWHEEL:
		{
			// The distance the mouse wheel is rotated.
			// A positive value indicates the wheel was rotated to the right.
			// A negative value indicates the wheel was rotated to the left.
			float zDelta = ((int)(short)HIWORD(wParam)) / (float)WHEEL_DELTA;
			short keyStates = (short)LOWORD(wParam);

			bool lButton = (wParam & MK_LBUTTON) != 0;
			bool rButton = (wParam & MK_RBUTTON) != 0;
			bool mButton = (wParam & MK_MBUTTON) != 0;
			bool shift = (wParam & MK_SHIFT) != 0;
			bool control = (wParam & MK_CONTROL) != 0;

			int x = ((int)(short)LOWORD(lParam));
			int y = ((int)(short)HIWORD(lParam));

			// Convert the screen coordinates to client coordinates
			POINT clientToScreenPoint;
			clientToScreenPoint.x = x;
			clientToScreenPoint.y = y;
			ScreenToClient(hWnd, &clientToScreenPoint);

			MouseWheelEventArgs mouseWheelEventArgs(zDelta, lButton, mButton, rButton, control, shift, (int)clientToScreenPoint.x, (int)clientToScreenPoint.y);
			window->OnMouseWheel(mouseWheelEventArgs);
		}
		break;

		case WM_SIZE:
		{
			int width = ((int)(short)LOWORD(lParam));
			int height = ((int)(short)HIWORD(lParam));

			ResizeEventArgs resizeEventArgs(width, height);
			window->OnResize(resizeEventArgs);
		}
		break;

		case WM_DESTROY:
		{
			// Remove window from lists
			RemoveWindow(hWnd);

			if (s_windows.empty())
			{
				PostQuitMessage(0);
			}
		}
		break;

		default:
			return DefWindowProcW(hWnd, message, wParam, lParam);
		}
	}
	else
	{
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}

	return 0;
}