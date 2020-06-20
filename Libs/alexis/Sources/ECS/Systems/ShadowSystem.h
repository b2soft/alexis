#pragma once

#include <DirectXMath.h>

#include <ECS/ECS.h>
#include <ECS/Components/CameraComponent.h>

namespace alexis
{
	class Mesh;
	class Material;
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
			Material* m_shadowMaterial{ nullptr };
			ecs::Entity m_phantomCamera;
		};
	}
}