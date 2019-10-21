#pragma once

#include <memory>
#include <d3d12.h>

#include "HighResolutionClock.h"

#include <imgui.h>
#include <imgui_impl_win32.h>

namespace alexis
{
	extern int g_clientWidth;
	extern int g_clientHeight;

	class IGame
	{
	public:
		virtual ~IGame() = default;

		virtual bool Initialize() { return true; }
		virtual void Destroy() { }

		virtual bool LoadContent() = 0;
		virtual void UnloadContent() = 0;

		virtual void OnUpdate(float dt) = 0;
		virtual void OnRender() = 0;
		//
		//virtual void OnKeyPressed(KeyEventArgs& e);
		//virtual void OnKeyReleased(KeyEventArgs& e);
		//virtual void OnMouseMoved(MouseMotionEventArgs& e);
		//virtual void OnMouseButtonPressed(MouseButtonEventArgs& e);
		//virtual void OnMouseButtonReleased(MouseButtonEventArgs& e);
		//virtual void OnMouseWheel(MouseWheelEventArgs& e);
		//
		virtual void OnResize(int width, int height) {};
		//virtual void OnWindowDestroy();
	};

	class Core
	{
	public:
		// Create application singleton
		static void Create(HINSTANCE hInstance);

		// Destroy app and windows
		static void Destroy();

		// Get the Core singleton
		static Core& Get();

		int Run(IGame* game);

		void Quit(int returnCode = 0);

		Core(const Core&) = delete;
		Core& operator=(const Core&) = delete;

		static HWND GetHwnd()
		{
			return s_hwnd;
		}
		
		static uint64_t GetFrameCount()
		{
			return s_frameCount;
		}

	protected:
		// Create app instance
		Core(HINSTANCE hInstance);

		void Initialize();

	private:
		friend class Render;
		friend LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
		friend extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		void CreateRenderWindow();
		void InitImgui();

		static HighResolutionClock s_updateClock;

		static HWND s_hwnd;
		static uint64_t s_frameCount;
		static IGame* s_game;
	};
}