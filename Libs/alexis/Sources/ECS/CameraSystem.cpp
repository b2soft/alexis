#include <Precompiled.h>

#include "CameraSystem.h"

#include <Core/Core.h>
#include <ECS/CameraComponent.h>
#include <ECS/TransformComponent.h>

namespace alexis
{
	namespace ecs
	{

		Entity CameraSystem::GetActiveCamera() const
		{
			// TODO: rework
			return *Entities.begin();
		}

		void XM_CALLCONV CameraSystem::SetPosition(Entity entity, DirectX::FXMVECTOR position)
		{
			auto ecsWorld = Core::Get().GetECS();
			auto& cameraComponent = ecsWorld->GetComponent<CameraComponent>(entity);
			auto& transformComponent = ecsWorld->GetComponent<TransformComponent>(entity);

			transformComponent.Position = position;

			cameraComponent.IsViewDirty = true;
			cameraComponent.IsInvViewDirty = true;
		}

		void XM_CALLCONV CameraSystem::SetRotation(Entity entity, DirectX::FXMVECTOR rotation)
		{
			auto ecsWorld = Core::Get().GetECS();
			auto& cameraComponent = ecsWorld->GetComponent<CameraComponent>(entity);
			auto& transformComponent = ecsWorld->GetComponent<TransformComponent>(entity);

			transformComponent.Rotation = rotation;

			cameraComponent.IsViewDirty = true;
			cameraComponent.IsInvViewDirty = true;
		}

		void XM_CALLCONV CameraSystem::SetTransform(Entity entity, DirectX::FXMVECTOR position, DirectX::FXMVECTOR rotation)
		{
			auto ecsWorld = Core::Get().GetECS();
			auto& cameraComponent = ecsWorld->GetComponent<CameraComponent>(entity);
			auto& transformComponent = ecsWorld->GetComponent<TransformComponent>(entity);

			transformComponent.Position = position;
			transformComponent.Rotation = rotation;

			cameraComponent.IsViewDirty = true;
			cameraComponent.IsInvViewDirty = true;
		}

		DirectX::XMMATRIX CameraSystem::GetViewMatrix(Entity entity) const
		{
			auto ecsWorld = Core::Get().GetECS();
			auto& cameraComponent = ecsWorld->GetComponent<CameraComponent>(entity);

			if (cameraComponent.IsViewDirty)
			{
				UpdateViewMatrix(entity);
			}

			return cameraComponent.CameraData->View;
		}

		DirectX::XMMATRIX CameraSystem::GetInvViewMatrix(Entity entity) const
		{
			auto ecsWorld = Core::Get().GetECS();
			auto& cameraComponent = ecsWorld->GetComponent<CameraComponent>(entity);

			if (cameraComponent.IsInvViewDirty)
			{
				UpdateInvViewMatrix(entity);
			}

			return cameraComponent.CameraData->InvView;
		}

		DirectX::XMMATRIX CameraSystem::GetProjMatrix(Entity entity) const
		{
			auto ecsWorld = Core::Get().GetECS();
			auto& cameraComponent = ecsWorld->GetComponent<CameraComponent>(entity);

			if (cameraComponent.IsProjDirty)
			{
				UpdateProjMatrix(entity);
			}

			return cameraComponent.CameraData->Proj;
		}

		DirectX::XMMATRIX CameraSystem::GetInvProjMatrix(Entity entity) const
		{
			auto ecsWorld = Core::Get().GetECS();
			auto& cameraComponent = ecsWorld->GetComponent<CameraComponent>(entity);

			if (cameraComponent.IsInvProjDirty)
			{
				UpdateInvProjMatrix(entity);
			}

			return cameraComponent.CameraData->InvProj;
		}

		void CameraSystem::SetFov(Entity entity, float fov)
		{
			auto ecsWorld = Core::Get().GetECS();
			auto& cameraComponent = ecsWorld->GetComponent<CameraComponent>(entity);

			if (!IsEqual(cameraComponent.Fov, fov))
			{
				cameraComponent.Fov = fov;
				cameraComponent.IsProjDirty = true;
				cameraComponent.IsInvProjDirty = true;
			}
		}

		void CameraSystem::SetProjectionParams(Entity entity, float fov, float aspectRatio, float nearZ, float farZ)
		{
			auto ecsWorld = Core::Get().GetECS();
			auto& cameraComponent = ecsWorld->GetComponent<CameraComponent>(entity);

			cameraComponent.Fov = fov;
			cameraComponent.AspectRatio = aspectRatio;
			cameraComponent.NearZ = nearZ;
			cameraComponent.FarZ = farZ;

			cameraComponent.IsProjDirty = true;
			cameraComponent.IsInvProjDirty = true;
		}

		void XM_CALLCONV CameraSystem::LookAt(Entity entity, DirectX::FXMVECTOR targetPos, DirectX::FXMVECTOR up)
		{
			auto ecsWorld = Core::Get().GetECS();
			auto& cameraComponent = ecsWorld->GetComponent<CameraComponent>(entity);
			auto& transformComponent = ecsWorld->GetComponent<TransformComponent>(entity);

			cameraComponent.CameraData->View = XMMatrixLookAtLH(transformComponent.Position, targetPos, up);
			transformComponent.Rotation = XMQuaternionRotationMatrix(XMMatrixTranspose(cameraComponent.CameraData->View));

			cameraComponent.IsViewDirty = false;
			cameraComponent.IsInvViewDirty = true;
		}

		void CameraSystem::UpdateViewMatrix(Entity entity) const
		{
			auto ecsWorld = Core::Get().GetECS();
			auto& cameraComponent = ecsWorld->GetComponent<CameraComponent>(entity);
			auto& transformComponent = ecsWorld->GetComponent<TransformComponent>(entity);

			XMMATRIX translationMatrix = XMMatrixTranslationFromVector(-(transformComponent.Position));
			XMMATRIX rotationMatrix = XMMatrixTranspose(XMMatrixRotationQuaternion(transformComponent.Rotation));

			cameraComponent.CameraData->View = translationMatrix * rotationMatrix;

			cameraComponent.IsInvViewDirty = true;
			cameraComponent.IsViewDirty = false;
		}

		void CameraSystem::UpdateInvViewMatrix(Entity entity) const
		{
			auto ecsWorld = Core::Get().GetECS();
			auto& cameraComponent = ecsWorld->GetComponent<CameraComponent>(entity);

			if (cameraComponent.IsViewDirty)
			{
				UpdateViewMatrix(entity);
			}

			cameraComponent.CameraData->InvView = XMMatrixInverse(nullptr, cameraComponent.CameraData->View);
			cameraComponent.IsInvViewDirty = false;
		}

		void CameraSystem::UpdateProjMatrix(Entity entity) const
		{
			auto ecsWorld = Core::Get().GetECS();
			auto& cameraComponent = ecsWorld->GetComponent<CameraComponent>(entity);

			if (cameraComponent.IsOrtho)
			{
				cameraComponent.CameraData->Proj = XMMatrixOrthographicLH(10.0f, 10.f, cameraComponent.NearZ, cameraComponent.FarZ);
				//cameraComponent.CameraData->View = XMMatrixLookAtLH(XMVECTOR{ -0.5, 0.8, 0.0, 1.0 }, XMVECTOR{ 0.0, 0.0, 0.0, 1.0 }, XMVECTOR{ 0.0, 1.0, 0.0, 0.0 });
			}
			else
			{
				cameraComponent.CameraData->Proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(cameraComponent.Fov), cameraComponent.AspectRatio, cameraComponent.NearZ, cameraComponent.FarZ);
			}

			cameraComponent.IsProjDirty = false;
			cameraComponent.IsInvProjDirty = true;
		}

		void CameraSystem::UpdateInvProjMatrix(Entity entity) const
		{
			auto ecsWorld = Core::Get().GetECS();
			auto& cameraComponent = ecsWorld->GetComponent<CameraComponent>(entity);

			if (cameraComponent.IsProjDirty)
			{
				UpdateProjMatrix(entity);
			}

			cameraComponent.CameraData->InvProj = XMMatrixInverse(nullptr, cameraComponent.CameraData->Proj);
			cameraComponent.IsInvProjDirty = false;
		}

	}
}
