#pragma once

namespace alexis
{
	class Core_Old;

	class Win32Application
	{
	public:
		static int Run(Core_Old* app, HINSTANCE hInstance, int nCmdShow);
		static HWND GetHwnd() { return s_hwnd; }

	protected:
		static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	private:
		static HWND s_hwnd;
	};
}