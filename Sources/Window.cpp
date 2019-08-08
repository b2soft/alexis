#include "Precompiled.h"

#include "Window.h"
#include "CommandQueue.h"
#include "Game.h"

#include "Application.h"

Window::Window(HWND hWnd, const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync)
	: m_hWnd(hWnd)
	, m_windowName(windowName)
	, m_clientWidth(clientWidth)
	, m_clientHeight(clientHeight)
	, m_vSync(vSync)
	, m_fullscreen(false)
	, m_frameCounter(0)
{
	Application& app = Application::Get();

	m_isTearingSupported = app.IsTearingSupported();

	m_dxgiSwapChain = CreateSwapChain();
	m_d3d12RTVDescriptorHeap = app.CreateDescriptorHeap(k_bufferCount, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_rtvDescriptorSize = app.GetDescriptorHeapIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	UpdateRenderTargetViews();
}

Window::~Window()
{
	// Window should be destroyed with Application::DestroyWindow before the window goes out of scope
	assert(!m_hWnd && "Use Application::DestroyWindow before Window destruction!");
}

void Window::RegisterCallbacks(std::shared_ptr<Game> game)
{
	m_game = game;
}

void Window::OnUpdate(UpdateEventArgs& e)
{
	m_updateClock.Tick();

	if (auto game = m_game.lock())
	{
		m_frameCounter++;

		UpdateEventArgs updateEventArgs(m_updateClock.GetDeltaSeconds(), m_updateClock.GetTotalSeconds());
		game->OnRender(updateEventArgs);
	}
}

void Window::OnRender(UpdateEventArgs& e)
{
	m_renderClock.Tick();

	if (auto game = m_game.lock())
	{
		RenderEventArgs renderEventArgs(m_renderClock.GetDeltaSeconds(), m_renderClock.GetTotalSeconds());
		game->OnRender(renderEventArgs);
	}
}

void Window::OnKeyPressed(KeyEventArgs& e)
{
	if (auto game = m_game.lock())
	{
		game->OnKeyPressed(e);
	}
}

void Window::OnKeyReleased(KeyEventArgs& e)
{
	if (auto game = m_game.lock())
	{
		game->OnKeyReleased(e);
	}
}

void Window::OnMouseMoved(MouseMotionEventArgs& e)
{
	if (auto game = m_game.lock())
	{
		game->OnMouseMoved(e);
	}
}

void Window::OnMouseButtonPressed(MouseButtonEventArgs& e)
{
	if (auto game = m_game.lock())
	{
		game->OnMouseButtonPressed(e);
	}
}

void Window::OnMouseButtonReleased(MouseButtonEventArgs& e)
{
	if (auto game = m_game.lock())
	{
		game->OnMouseButtonReleased(e);
	}
}

void Window::OnMouseWheel(MouseWheelEventArgs& e)
{
	if (auto game = m_game.lock())
	{
		game->OnMouseWheel(e);
	}
}

void Window::OnResize(ResizeEventArgs& e)
{
	if (m_clientWidth != e.Width || m_clientHeight != e.Height)
	{
		m_clientWidth = std::max(1, e.Width);
		m_clientHeight = std::max(1, e.Height);

		Application::Get().Flush();

		for (int i = 0; i < k_bufferCount; ++i)
		{
			m_d3d12BackBuffers[i].Reset();
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		ThrowIfFailed(m_dxgiSwapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(m_dxgiSwapChain->ResizeBuffers(k_bufferCount, m_clientWidth, m_clientHeight, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

		m_currentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();

		UpdateRenderTargetViews();
	}

	if (auto game = m_game.lock())
	{
		game->OnResize(e);
	}
}

Microsoft::WRL::ComPtr<IDXGISwapChain4> Window::CreateSwapChain()
{
	ComPtr<IDXGISwapChain4> dxgiSwapChain4;
	ComPtr<IDXGIFactory4> dxgiFactory4;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = m_clientWidth;
	swapChainDesc.Height = m_clientHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = k_bufferCount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	// It is recommended to always allow tearing if tearing support is available.
	swapChainDesc.Flags = m_isTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	Application& app = Application::Get();
	ID3D12CommandQueue* commandQueue = app.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetD3D12CommandQueue().Get();

	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(commandQueue, m_hWnd, &swapChainDesc, nullptr, nullptr, &swapChain1));

	// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
   // will be handled manually.
	ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain1.As(&dxgiSwapChain4));

	m_currentBackBufferIndex = dxgiSwapChain4->GetCurrentBackBufferIndex();

	return dxgiSwapChain4;
}

void Window::UpdateRenderTargetViews()
{
	auto device = Application::Get().GetDevice();

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_d3d12RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < k_bufferCount; ++i)
	{
		ComPtr<ID3D12Resource> backBuffer;
		ThrowIfFailed(m_dxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

		m_d3d12BackBuffers[i] = backBuffer;

		rtvHandle.Offset(m_rtvDescriptorSize);
	}
}

HWND Window::GetWindowHandle() const
{
	return m_hWnd;
}

void Window::Destroy()
{
	if (auto game = m_game.lock())
	{
		// Notify Game about window destroying
		game->OnWindowDestroy();
	}

	if (m_hWnd)
	{
		DestroyWindow(m_hWnd);
		m_hWnd = nullptr;
	}
}

const std::wstring& Window::GetWindowName() const
{
	return m_windowName;
}

int Window::GetClientWidth() const
{
	return m_clientWidth;
}

int Window::GetClientHeight() const
{
	return m_clientHeight;
}

bool Window::IsVSync() const
{
	return m_vSync;
}

void Window::SetVSync(bool vSync)
{
	m_vSync = vSync;
}

void Window::ToggleVSync()
{
	SetVSync(!m_vSync);
}

bool Window::IsFullscreen() const
{
	return m_fullscreen;
}

void Window::SetFullscreen(bool fullscreen)
{
	if (m_fullscreen != fullscreen)
	{
		m_fullscreen = fullscreen;

		if (m_fullscreen)
		{
			// Store the current window dimensions so they can be restored 
			// when switching out of fullscreen state.
			::GetWindowRect(m_hWnd, &m_windowRect);

			//Set window to be borderless
			UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

			::SetWindowLongW(m_hWnd, GWL_STYLE, windowStyle);

			// Query the name of the nearest display device for the window.
			// This is required to set the fullscreen dimensions of the window
			// when using a multi-monitor setup.
			HMONITOR hMonitor = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFOEX monitorInfo = {};
			monitorInfo.cbSize = sizeof(MONITORINFOEX);
			::GetMonitorInfo(hMonitor, &monitorInfo);

			::SetWindowPos(m_hWnd, HWND_TOP,
				monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.top,
				monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE);

			::ShowWindow(m_hWnd, SW_MAXIMIZE);
		}
		else
		{
			// Restore all window decorators
			::SetWindowLong(m_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

			::SetWindowPos(m_hWnd, HWND_NOTOPMOST,
				m_windowRect.left,
				m_windowRect.top,
				m_windowRect.right - m_windowRect.left,
				m_windowRect.bottom - m_windowRect.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE);

			::ShowWindow(m_hWnd, SW_NORMAL);
		}
	}
}

void Window::ToggleFullscreen()
{
	SetFullscreen(!m_fullscreen);
}

void Window::Show()
{
	ShowWindow(m_hWnd, SW_SHOW);
}

void Window::Hide()
{
	ShowWindow(m_hWnd, SW_HIDE);
}

UINT Window::GetCurrentBackBufferIndex() const
{
	return m_currentBackBufferIndex;
}

UINT Window::Present()
{
	UINT syncInterval = m_vSync ? 1 : 0;
	UINT presetFlags = m_isTearingSupported && !m_vSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	ThrowIfFailed(m_dxgiSwapChain->Present(syncInterval, presetFlags));
	m_currentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();

	return m_currentBackBufferIndex;
}

D3D12_CPU_DESCRIPTOR_HANDLE Window::GetCurrentRenderTargetView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_d3d12RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_currentBackBufferIndex, m_rtvDescriptorSize);
}

Microsoft::WRL::ComPtr<ID3D12Resource> Window::GetCurrentBackBuffer() const
{
	return m_d3d12BackBuffers[m_currentBackBufferIndex];
}

