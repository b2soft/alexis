#pragma once

namespace alexis
{
	class CoreApplication;

	class Win32Application
	{
	public:
		static int Run(CoreApplication* app, HINSTANCE hInstance, int nCmdShow);
		static HWND GetHwnd() { return s_hwnd; }

	protected:
		static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	private:
		static HWND s_hwnd;
	};
}