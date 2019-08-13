#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_5.h>
#include <string>
#include <memory>

#include "Events.h"
#include "HighResolutionClock.h"

#include "Render/Texture.h"
#include "Render/RenderTarget.h"

class Game;

class Window
{
public:
	static const UINT k_bufferCount = 2;

	HWND GetWindowHandle() const;

	void Initialize();
	void Destroy();

	const std::wstring& GetWindowName() const;

	int GetClientWidth() const;
	int GetClientHeight() const;

	bool IsVSync() const;
	void SetVSync(bool vSync);
	void ToggleVSync();

	bool IsFullscreen() const;
	void SetFullscreen(bool fullscreen);
	void ToggleFullscreen();

	void Show();
	void Hide();

	const RenderTarget& GetRenderTarget() const;

	// Present the swapchain's back buffer to the screen
	// Returns the current back buffer index after the present
	// By default, this is an empty texture. In this case, no copy will be performed
	// Use the Window::GetRenderTarget method to get a render target for the window's color buffer
	UINT Present(const Texture& texture = Texture());

protected:
	friend LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	// Only the Application can create a window
	friend class Application;

	// Game class needs to register itself with a window
	friend class Game;

	Window() = delete;
	Window(HWND hWnd, const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync);
	virtual ~Window();

	// Register a Game with this window. This allows the window to callback functions in the Game class
	void RegisterCallbacks(std::shared_ptr<Game> game);

	// Update and Draw
	virtual void OnUpdate(UpdateEventArgs& e);
	virtual void OnRender(RenderEventArgs& e);

	// Keyboard
	virtual void OnKeyPressed(KeyEventArgs& e);
	virtual void OnKeyReleased(KeyEventArgs& e);

	// Mouse
	virtual void OnMouseMoved(MouseMotionEventArgs& e);
	virtual void OnMouseButtonPressed(MouseButtonEventArgs& e);
	virtual void OnMouseButtonReleased(MouseButtonEventArgs& e);
	virtual void OnMouseWheel(MouseWheelEventArgs& e);

	virtual void OnResize(ResizeEventArgs& e);

	Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChain();

	void UpdateRenderTargetViews();

private:
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	HWND m_hWnd;

	std::wstring m_windowName;

	int m_clientWidth;
	int m_clientHeight;
	bool m_vSync;
	bool m_fullscreen;

	HighResolutionClock m_updateClock;
	HighResolutionClock m_renderClock;
	
	UINT64 m_fenceValues[k_bufferCount];
	uint64_t m_frameValues[k_bufferCount];

	std::weak_ptr<Game> m_game;

	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_dxgiSwapChain;
	Texture m_backBufferTextures[k_bufferCount];

	mutable RenderTarget m_renderTarget;

	UINT m_currentBackBufferIndex;

	RECT m_windowRect;
	bool m_isTearingSupported;

	// TODO: Further imGUI impl
	// int m_PreviousMouseX;
	// int m_PreviousMouseY;
	// 
	// GUI m_GUI;
};