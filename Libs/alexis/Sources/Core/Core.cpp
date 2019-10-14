#include <Precompiled.h>

#include "Core.h"

#include <string>

#include "Render.h"

static Core* s_singleton = nullptr;
static HINSTANCE s_hInstance;

static int g_clientWidth = 1280;
static int g_clientHeight = 720;

static constexpr wchar_t k_windowClassName[] = L"AlexisWindowClass";
static constexpr wchar_t k_windowTitle[] = L"Alexis App";

HWND Core::s_hwnd = nullptr;
uint64_t Core::s_frameCount = 0;
std::shared_ptr<IGame> Core::s_game;

static LRESULT CALLBACK WindowProc2(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

void Core::CreateRenderWindow()
{
	RECT windowRect = { 0, 0, static_cast<LONG>(g_clientWidth), static_cast<LONG>(g_clientHeight) };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc2;
	windowClass.hInstance = s_hInstance;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = k_windowClassName;

	if (!RegisterClassExW(&windowClass))
	{
		MessageBoxA(NULL, "Unable to register window class!", "Error", MB_OK | MB_ICONERROR);
	}

	Core::s_hwnd = CreateWindowW(
		windowClass.lpszClassName,
		k_windowTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr, // no parent
		nullptr, // nor menus
		s_hInstance,
		this);

	if (!s_hwnd)
	{
		//DWORD errorMessageID = ::GetLastError();

		//LPSTR messageBuffer = nullptr;
		//size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		//	NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

		//std::string message(messageBuffer, size);

		////Free the buffer.
		//LocalFree(messageBuffer);

		MessageBoxA(NULL, "Could not create the window for rendering", "Error", MB_OK | MB_ICONERROR);
	}
}

void Core::Create(HINSTANCE hInstance)
{
	if (!s_singleton)
	{
		s_singleton = new Core(hInstance);
		s_singleton->Initialize();
	}
}

void Core::Destroy()
{
	Render::GetInstance()->Destroy();
	Render::DestroyInstance();

	s_game->UnloadContent();
	s_game->Destroy();

	s_game.reset();

	s_hwnd = nullptr;

	if (s_singleton)
	{
		delete s_singleton;
		s_singleton = nullptr;
	}
}

Core& Core::Get()
{
	assert(s_singleton);
	return *s_singleton;
}

int Core::Run(std::shared_ptr<IGame> game)
{
	s_game = game;

	if (!s_game->Initialize())
	{
		return 1;
	}

	if (!s_game->LoadContent())
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

	//Graphics Flush
	s_game->UnloadContent();
	s_game->Destroy();

	return static_cast<int>(msg.wParam);
}

void Core::Quit(int returnCode /*= 0*/)
{
	PostQuitMessage(returnCode);
}

Core::Core(HINSTANCE hInstance)
{
	// Windows 10 Creators update adds Per Monitor V2 DPI awareness context.
	// Using this awareness context allows the client area of the window 
	// to achieve 100% scaling while still allowing non-client window content to 
	// be rendered in a DPI sensitive fashion.
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	s_hInstance = hInstance;
}

void Core::Initialize()
{
	CreateRenderWindow();
	ShowWindow(s_hwnd, TRUE);

	// Check for DirectX Math lib support
	if (!DirectX::XMVerifyCPUSupport())
	{
		MessageBoxA(NULL, "Failed to verify DirectX Math library support!", "Error", MB_OK | MB_ICONERROR);
	}

	Render::GetInstance()->Initialize(g_clientWidth, g_clientHeight);

	s_frameCount = 0;
}

static LRESULT CALLBACK WindowProc2(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
	//{
	//	return true;
	//}

	if (!Core::s_hwnd)
	{
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}

	switch (message)
	{
	case WM_PAINT:
	{
		++Core::s_frameCount;

		Core::s_game->OnUpdate(0.0f);


		auto render = Render::GetInstance();
		// Reset queue and lists
		render->BeginRender();

		// Populate game command lists to render
		Core::s_game->OnRender();

		// Present to screen
		render->Present();
	}
	break;

	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	{
		//MSG charMsg;
		//// Get the Unicode utf-16 char
		//unsigned int c = 0;

		//// For printable characters, the next message will be WM_CHAR.
		//// This message contains the character code we need to send the KeyPressed event.
		//// Inspired by the SDL 1.2 implementation.
		//if (PeekMessageW(&charMsg, hWnd, 0, 0, PM_NOREMOVE) && charMsg.message == WM_CHAR)
		//{
		//	GetMessage(&charMsg, hWnd, 0, 0);
		//	c = static_cast<unsigned int>(charMsg.wParam);

		//	if (charMsg.wParam > 0 && charMsg.wParam < 0x10000)
		//	{
		//		ImGui::GetIO().AddInputCharacter((unsigned short)charMsg.wParam);
		//	}
		//}

		//bool shift = (GetAsyncKeyState(VK_SHIFT & 0x8000) != 0);
		//bool control = (GetAsyncKeyState(VK_CONTROL & 0x8000) != 0);
		//bool alt = (GetAsyncKeyState(VK_MENU & 0x8000) != 0);
		//KeyCode::Key key = (KeyCode::Key)wParam;
		//unsigned int scanCode = (lParam & 0x00FF0000) >> 16;
		//KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Pressed, shift, control, alt);
		//window->OnKeyPressed(keyEventArgs);
	}
	break;

	case WM_SYSKEYUP:
	case WM_KEYUP:
	{
		//bool shift = (GetAsyncKeyState(VK_SHIFT & 0x8000) != 0);
		//bool control = (GetAsyncKeyState(VK_CONTROL & 0x8000) != 0);
		//bool alt = (GetAsyncKeyState(VK_MENU & 0x8000) != 0);
		//KeyCode::Key key = (KeyCode::Key)wParam;
		//unsigned int c = 0;
		//unsigned int scanCode = (lParam & 0x00FF0000) >> 16;

		//// Determine which key was released by converting the key code and the scan code
		//// to a printable character (if possible).
		//// Inspired by the SDL 1.2 implementation.
		//unsigned char keyboardState[256];
		//GetKeyboardState(keyboardState);
		//wchar_t translatedCharacters[4];
		//if (ToUnicodeEx(static_cast<UINT>(wParam), scanCode, keyboardState, translatedCharacters, 4, 0, NULL) > 0)
		//{
		//	c = translatedCharacters[0];
		//}

		//KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Released, shift, control, alt);
		//window->OnKeyReleased(keyEventArgs);
	}
	break;

	// The default window procedure will play a system notification sound 
	// when pressing the Alt+Enter keyboard combination if this message is not handled
	case WM_SYSCHAR:
		break;

	case WM_MOUSEMOVE:
	{
		//bool lButton = (wParam & MK_LBUTTON) != 0;
		//bool rButton = (wParam & MK_RBUTTON) != 0;
		//bool mButton = (wParam & MK_MBUTTON) != 0;
		//bool shift = (wParam & MK_SHIFT) != 0;
		//bool control = (wParam & MK_CONTROL) != 0;

		//int x = ((int)(short)LOWORD(lParam));
		//int y = ((int)(short)HIWORD(lParam));

		//MouseMotionEventArgs mouseMotionEventArgs(lButton, mButton, rButton, control, shift, x, y);
		//window->OnMouseMoved(mouseMotionEventArgs);
	}
	break;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	{
		//bool lButton = (wParam & MK_LBUTTON) != 0;
		//bool rButton = (wParam & MK_RBUTTON) != 0;
		//bool mButton = (wParam & MK_MBUTTON) != 0;
		//bool shift = (wParam & MK_SHIFT) != 0;
		//bool control = (wParam & MK_CONTROL) != 0;

		//int x = ((int)(short)LOWORD(lParam));
		//int y = ((int)(short)HIWORD(lParam));

		//MouseButtonEventArgs mouseButtonEventArgs(DecodeMouseButton(message), MouseButtonEventArgs::Pressed, lButton, mButton, rButton, control, shift, x, y);
		//window->OnMouseButtonPressed(mouseButtonEventArgs);
	}
	break;

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	{
		//bool lButton = (wParam & MK_LBUTTON) != 0;
		//bool rButton = (wParam & MK_RBUTTON) != 0;
		//bool mButton = (wParam & MK_MBUTTON) != 0;
		//bool shift = (wParam & MK_SHIFT) != 0;
		//bool control = (wParam & MK_CONTROL) != 0;

		//int x = ((int)(short)LOWORD(lParam));
		//int y = ((int)(short)HIWORD(lParam));

		//MouseButtonEventArgs mouseButtonEventArgs(DecodeMouseButton(message), MouseButtonEventArgs::Released, lButton, mButton, rButton, control, shift, x, y);
		//window->OnMouseButtonReleased(mouseButtonEventArgs);
	}
	break;

	case WM_MOUSEWHEEL:
	{
		// The distance the mouse wheel is rotated.
		// A positive value indicates the wheel was rotated to the right.
		// A negative value indicates the wheel was rotated to the left.
		//float zDelta = ((int)(short)HIWORD(wParam)) / (float)WHEEL_DELTA;
		//short keyStates = (short)LOWORD(wParam);

		//bool lButton = (wParam & MK_LBUTTON) != 0;
		//bool rButton = (wParam & MK_RBUTTON) != 0;
		//bool mButton = (wParam & MK_MBUTTON) != 0;
		//bool shift = (wParam & MK_SHIFT) != 0;
		//bool control = (wParam & MK_CONTROL) != 0;

		//int x = ((int)(short)LOWORD(lParam));
		//int y = ((int)(short)HIWORD(lParam));

		//// Convert the screen coordinates to client coordinates
		//POINT clientToScreenPoint;
		//clientToScreenPoint.x = x;
		//clientToScreenPoint.y = y;
		//ScreenToClient(hWnd, &clientToScreenPoint);

		//MouseWheelEventArgs mouseWheelEventArgs(zDelta, lButton, mButton, rButton, control, shift, (int)clientToScreenPoint.x, (int)clientToScreenPoint.y);
		//window->OnMouseWheel(mouseWheelEventArgs);
	}
	break;

	case WM_SIZE:
	{
		g_clientWidth = ((int)(short)LOWORD(lParam));
		g_clientHeight = ((int)(short)HIWORD(lParam));

		if (Core::s_game)
		{
			Render::GetInstance()->OnResize(g_clientWidth, g_clientHeight);
			Core::s_game->OnResize(g_clientWidth, g_clientHeight);
		}
	}
	break;

	case WM_DESTROY:
	{
		Core::Get().Quit();
		return 0;
	}
	break;

	default:
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}

	return 0;
}
