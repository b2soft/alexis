#pragma once

#include <memory>
#include <string>

#include "Events.h"

class Window;

class Game : public std::enable_shared_from_this<Game>
{
public:
	Game(const std::wstring& name, int width, int height, bool vSync);
	virtual ~Game();

	// Warning! Has to be refined!
	// Name clashing with Window::GetClientWidth
	// Window size is changed immediately
	// This sizes represent render rect size
	int GetClientWidth() const
	{
		return m_width;
	}

	int GetClientHeight() const
	{
		return m_height;
	}

	// End of warning

	virtual bool Initialize();
	virtual void Destroy();

	virtual bool LoadContent() = 0;
	virtual void UnloadContent() = 0;

protected:
	friend class Window;

	virtual void OnUpdate(UpdateEventArgs& e);
	virtual void OnRender(RenderEventArgs& e);

	virtual void OnKeyPressed(KeyEventArgs& e);
	virtual void OnKeyReleased(KeyEventArgs& e);
	virtual void OnMouseMoved(MouseMotionEventArgs& e);
	virtual void OnMouseButtonPressed(MouseButtonEventArgs& e);
	virtual void OnMouseButtonReleased(MouseButtonEventArgs& e);
	virtual void OnMouseWheel(MouseWheelEventArgs& e);

	virtual void OnResize(ResizeEventArgs& e);
	virtual void OnWindowDestroy();

	std::shared_ptr<Window> m_window;

private:
	std::wstring m_name;
	int m_width;
	int m_height;
	bool m_vSync;
};