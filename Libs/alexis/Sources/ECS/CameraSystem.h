#pragma once

#include <ECS/ECS.h>

namespace alexis
{
	namespace ecs
	{
		class CameraSystem : public ecs::System
		{
		public:
			void XM_CALLCONV SetPosition(Entity entity, DirectX::FXMVECTOR position);
			void XM_CALLCONV SetRotation(Entity entity, DirectX::FXMVECTOR rotation);
			void XM_CALLCONV SetTransform(Entity entity, DirectX::FXMVECTOR position, DirectX::FXMVECTOR rotation);

			DirectX::XMMATRIX GetViewMatrix(Entity entity) const;
			DirectX::XMMATRIX GetInvViewMatrix(Entity entity) const;

			DirectX::XMMATRIX GetProjMatrix(Entity entity) const;
			DirectX::XMMATRIX GetInvProjMatrix(Entity entity) const;

			void SetFov(Entity entity, float fov);
			void SetProjectionParams(Entity entity, float fov, float aspectRatio, float nearZ, float farZ);
			void XM_CALLCONV LookAt(Entity entity, DirectX::FXMVECTOR targetPos, DirectX::FXMVECTOR up);

		private:
			void UpdateViewMatrix(Entity entity) const;
			void UpdateInvViewMatrix(Entity entity) const;

			void UpdateProjMatrix(Entity entity) const;
			void UpdateInvProjMatrix(Entity entity) const;
		};
	}
}