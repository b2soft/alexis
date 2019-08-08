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

class Game;

class Window
{
public:
	static const UINT k_bufferCount = 2;

	HWND GetWindowHandle() const;
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

	// Return the current back buffer index
	UINT GetCurrentBackBufferIndex() const;

	// Present the swapchain's back buffer to the screen
	// Returns the current back buffer index after the present
	UINT Present();

	// Get the render target view for the current back buffer
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetView() const;

	// Get the back buffer resource for the current back buffer
	Microsoft::WRL::ComPtr<ID3D12Resource> GetCurrentBackBuffer() const;

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
	virtual void OnRender(UpdateEventArgs& e);

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
	uint64_t m_frameCounter;

	std::weak_ptr<Game> m_game;

	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_dxgiSwapChain;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_d3d12RTVDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_d3d12BackBuffers[k_bufferCount];

	UINT m_rtvDescriptorSize;
	UINT m_currentBackBufferIndex;

	RECT m_windowRect;
	bool m_isTearingSupported;
};