#pragma once

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

	int Run(std::shared_ptr<IGame> game);

	void Quit(int returnCode = 0);

	Core(const Core&) = delete;
	Core& operator=(const Core&) = delete;

protected:
	// Create app instance
	Core(HINSTANCE hInstance);

	void Initialize();

private:
	friend LRESULT CALLBACK WindowProc2(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	void CreateRenderWindow();

	static uint64_t s_frameCount;
	static std::shared_ptr<IGame> s_game;
};