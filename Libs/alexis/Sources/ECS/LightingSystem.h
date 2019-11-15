#pragma once

#include <ECS/ECS.h>

namespace alexis
{
	class MaterialBase;
	class CommandContext;
	class Mesh;

	namespace ecs
	{
		class LightingSystem : public System
		{
		public:
			void Init();
			void Render(CommandContext* context);

		private:
			std::unique_ptr<MaterialBase> m_lightingMaterial;
			Mesh* m_fsQuad{ nullptr };
		};
	}
}