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
			DirectX::XMVECTOR GetSunDirection() const;

		private:
			Material* m_lightingMaterial{ nullptr };
			Mesh* m_fsQuad{ nullptr };
		};
	}
}