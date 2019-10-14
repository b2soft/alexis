#include <Precompiled.h>

#include "Win32Application.h"

#include "Core_old.h"

namespace alexis
{

	HWND Win32Application::s_hwnd = nullptr;

	int Win32Application::Run(Core_Old* app, HINSTANCE hInstance, int nCmdShow)
	{
		// Parse command list args first
		int argc = 0;
		LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
		app->ParseCommandLineArgs(argv, argc);
		LocalFree(argv);

		// Init window class
		WNDCLASSEX windowClass = { 0 };
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = WindowProc;
		windowClass.hInstance = hInstance;
		windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		windowClass.lpszClassName = L"AlexisWindowClass";

		RegisterClassEx(&windowClass);

		RECT windowRect = { 0, 0, static_cast<LONG>(app->GetWidth()), static_cast<LONG>(app->GetHeight()) };
		AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

		s_hwnd = CreateWindowW(
			windowClass.lpszClassName,
			app->GetTitle(),
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			nullptr, // no parent
			nullptr, // nor menus
			hInstance,
			app);

		app->OnInit();

		ShowWindow(s_hwnd, nCmdShow);

		// Main OS msg loop
		MSG msg = {};
		while (msg.message != WM_QUIT)
		{
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		app->OnDestroy();

		// Return WM_QUIT param to OS
		return static_cast<char>(msg.wParam);
	}

	LRESULT CALLBACK Win32Application::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		Core_Old* app = reinterpret_cast<Core_Old*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

		switch (message)
		{
		case WM_CREATE:
		{
			// Save the CoreApplication* passed in to CreateWindow
			LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
		}
		return 0;

		case WM_KEYDOWN:
		{
			if (app)
			{
				app->OnKeyDown(static_cast<UINT8>(wParam));
			}
			return 0;
		}

		case WM_KEYUP:
		{
			if (app)
			{
				app->OnKeyUp(static_cast<UINT8>(wParam));
			}
			return 0;
		}

		case WM_PAINT:
		{
			if (app)
			{
				app->OnUpdate();
				app->OnRender();
			}
			return 0;
		}

		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}

		}

		// Handle any messages the switch statement did not
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

}
