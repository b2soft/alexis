#pragma once

namespace alexis
{
	namespace ecs
	{
		class ModelSystem;
		class ShadowSystem;
		class CameraSystem;
		class LightingSystem;
		class Hdr2SdrSystem;
		class ImguiSystem;
		class EnvironmentSystem;
		class EditorSystem;
	}

	class SystemsHolder
	{
	public:
		void Init();

	private:
		void Register();
		void InitInternal();

		std::shared_ptr<alexis::ecs::ModelSystem> m_modelSystem;
		std::shared_ptr<alexis::ecs::ShadowSystem> m_shadowSystem;
		std::shared_ptr<alexis::ecs::CameraSystem> m_cameraSystem;
		std::shared_ptr<alexis::ecs::LightingSystem> m_lightingSystem;
		std::shared_ptr<alexis::ecs::Hdr2SdrSystem> m_hdr2SdrSystem;
		std::shared_ptr<alexis::ecs::ImguiSystem> m_imguiSystem;
		std::shared_ptr<alexis::ecs::EnvironmentSystem> m_environmentSystem;
		std::shared_ptr<alexis::ecs::EditorSystem> m_editorSystem;
	};
}
