#pragma once

#include <DirectXMath.h>

#include <ECS/ECS.h>
#include <ECS/CameraComponent.h>

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

			DirectX::XMMATRIX GetShadowMatrix() const;

		private:
			std::unique_ptr<MaterialBase> m_shadowMaterial;
			ecs::Entity m_phantomCamera;
		};
	}
}