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
			void Update(float dt);

		private:
			std::optional<Scene> m_loadedScene;
		};

	}
}