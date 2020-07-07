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
			void PointLights(CommandContext* context);
			void AmbientLight(CommandContext* context);
			DirectX::XMVECTOR GetSunDirection() const;

		private:
			Mesh* m_fsQuad{ nullptr };

			Mesh* m_sphere{ nullptr };


			std::unique_ptr<Material> m_pointLightStencil;
			std::unique_ptr<Material> m_pointLight;
			std::unique_ptr<Material> m_ambientLight;
		};
	}
}