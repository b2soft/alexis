#pragma once

#include <ECS/ECS.h>

namespace alexis
{
	class Material;
	class CommandContext;
	class Mesh;

	namespace ecs
	{
		class LightingSystem : public System
		{
		public:
			void Init();
			void Render(CommandContext* context);
			void Render2(CommandContext* context);
			DirectX::XMVECTOR GetSunDirection() const;

		private:
			Material* m_lightingMaterial{ nullptr };
			Mesh* m_fsQuad{ nullptr };

			Mesh* m_sphere{ nullptr };

			std::unique_ptr<Material> m_lightStencil;
			std::unique_ptr<Material> m_lightShading;
		};
	}
}