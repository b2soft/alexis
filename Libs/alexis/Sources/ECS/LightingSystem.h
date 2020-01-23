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
			DirectX::XMVECTOR GetSunDirection() const;

		private:
			std::unique_ptr<MaterialBase> m_lightingMaterial;
			Mesh* m_fsQuad{ nullptr };
		};
	}
}