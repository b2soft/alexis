#include "Precompiled.h"

#include "Application.h"
#include "Window.h"

#include "Game.h"

Game::Game(const std::wstring& name, int width, int height, bool vSync)
	: m_name(name)
	, m_width(width)
	, m_height(height)
	, m_vSync(vSync)
{
}

Game::~Game()
{
	assert(!m_window && "Use Game::Destroy() before destruction!");
}

bool Game::Initialize()
{
	// Check for DirectX Math lib support
	if (!DirectX::XMVerifyCPUSupport())
	{
		MessageBoxA(NULL, "failed to verify DirectX Math library support!", "Error", MB_OK | MB_ICONERROR);
		return false;
	}

	m_window = Application::Get().CreateRenderWindow(m_name, m_width, m_height, m_vSync);
	m_window->RegisterCallbacks(shared_from_this());
	m_window->Show();

	return true;
}

void Game::Destroy()
{
	Application::Get().DestroyWindow(m_window);
	m_window.reset();
}

void Game::OnUpdate(UpdateEventArgs& e)
{

}

void Game::OnRender(RenderEventArgs& e)
{

}

void Game::OnKeyPressed(KeyEventArgs& e)
{

}

void Game::OnKeyReleased(KeyEventArgs& e)
{

}

void Game::OnMouseMoved(MouseMotionEventArgs& e)
{

}

void Game::OnMouseButtonPressed(MouseButtonEventArgs& e)
{

}

void Game::OnMouseButtonReleased(MouseButtonEventArgs& e)
{

}

void Game::OnMouseWheel(MouseWheelEventArgs& e)
{

}

void Game::OnResize(ResizeEventArgs& e)
{
	m_width = e.Width;
	m_height = e.Height;
}

void Game::OnWindowDestroy()
{
	// If the Window which we are registered to is destroyed, then any resources which are associated to the window must be released
	UnloadContent();
}
