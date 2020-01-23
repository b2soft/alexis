#pragma once

#include <DirectXMath.h>

#include <ECS/ECS.h>

namespace alexis
{
	class Mesh;
	class MaterialBase;
	class CommandContext;

	namespace ecs
	{
		class ShadowSystem : public ecs::System
		{
		public:
			void Init();
			void XM_CALLCONV Render(CommandContext* context);

		private:
			std::unique_ptr<MaterialBase> m_shadowMaterial;
		};
	}
}