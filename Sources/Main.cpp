#include "Precompiled.h"

HWND g_hwnd = nullptr;

const LPCTSTR k_windowName = L"Render DX12";
const LPCTSTR k_windowTitle = L"Window";

const int k_width = 800;
const int k_height = 600;

const bool k_fullScreen = false;

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

	while (true)
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
			if (MessageBox(0, L"Are you sutr you want to exit?", L"Really?", MB_YESNO | MB_ICONQUESTION) == IDYES)
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

	MainLoop();






	return 0;
}