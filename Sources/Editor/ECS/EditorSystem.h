#pragma once

#include <ECS/ECS.h>

namespace editor
{
	class EditorSystem : public alexis::ecs::System
	{
	public:
		void Init();
		void Update(float dt);

		void OnResize(int width, int height);
		void OnMouseMoved(int x, int y);
		void SetMovement(const std::array<float, 4>& movement);

		alexis::ecs::Entity GetActiveCamera() const;
		std::set<alexis::ecs::Entity> GetEntityList() const;

		bool IsCameraFixed() const;
		void ToggleFixedCamera();

		// TODO: redo this using quat from TransformComponent (and use relative pitch yaw)
		float m_pitch{ 0.0f };
		float m_yaw{ 0.0 };
		void UpdateCameraTransform(float dt);

	private:
		void UpdateEditorCamera(float dt);
		void UpdateECSGUI();

		alexis::ecs::Entity m_editorCamera{ 0 };

		std::array<float, 4> m_movement{ 0.f, 0.f, 0.f, 0.f };

		int m_deltaMouseX{ 0 };
		int m_deltaMouseY{ 0 };
		bool m_isCameraFixed{ true };
	};

}