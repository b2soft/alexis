#pragma once

#include <optional>

#include <Core/Core.h>
#include <Scene.h>

#include "ECS/EditorSystem.h"

class EditorApp : public alexis::IGame
{
public:
	bool Initialize() override;
	void OnResize(int width, int height) override;
	void OnUpdate(float dt) override;
	void OnRender(float frameTime) override;

	void OnKeyPressed(alexis::KeyEventArgs& e) override;
	void OnKeyReleased(alexis::KeyEventArgs& e) override;

	void OnMouseMoved(alexis::MouseMotionEventArgs& e) override;

	void LoadScene(std::wstring_view path);
	void UnloadScene();
	void SaveScene();

private:
	void ResetMousePos();
	void ToggleFixedCamera();

	std::shared_ptr<editor::EditorSystem> m_editorSystem;

	std::optional<alexis::Scene> m_loadedScene;

	// FPS & UPS
	float m_frameTimeCount{ 0.f };
	int m_frameCount{ 0 };
	float m_fps{ 0.f };

	float m_updateTimeCount{ 0.f };
	int m_updateCount{ 0 };
	float m_ups{ 0.f };
	
	
	std::array<float, 4> m_movement{ 0.f, 0.f, 0.f, 0.f }; // fwd aft left right
};