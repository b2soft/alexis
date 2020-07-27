#pragma once

#include <ECS/ECS.h>
#include <optional>
#include <Scene.h>

namespace alexis
{
	namespace ecs
	{
		class EditorSystem : public System
		{
		public:
			void LoadScene(std::wstring_view path);
			void UnloadScene();
			void SaveScene();

			void Init();
			void Update(float dt);

			void OnResize(int width, int height);

			ecs::Entity GetActiveCamera() const;

		private:
			std::optional<Scene> m_loadedScene;
			ecs::Entity m_editorCamera{ 0 };
		};

	}
}