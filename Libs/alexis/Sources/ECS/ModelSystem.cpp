#include <Precompiled.h>

#include "ModelSystem.h"

#include <Core/Core.h>
#include <Render/Mesh.h>
#include <Render/CommandContext.h>
#include <Render/Materials/MaterialBase.h>

#include <ECS/CameraSystem.h>

#include <ECS/CameraComponent.h>
#include <ECS/ModelComponent.h>
#include <ECS/TransformComponent.h>

namespace alexis
{
	namespace ecs
	{
		struct CameraCB
		{
			XMMATRIX viewProjMatrix;
		};

		void ModelSystem::Update(float dt)
		{
			auto ecsWorld = Core::Get().GetECS();
			for (const auto& entity : Entities)
			{
				auto& modelComponent = ecsWorld->GetComponent<ModelComponent>(entity);
				auto& transformComponent = ecsWorld->GetComponent<TransformComponent>(entity);

				if (modelComponent.IsTransformDirty)
				{
					XMMATRIX translationMatrix = XMMatrixTranslationFromVector(transformComponent.Position);
					XMMATRIX rotationMatrix = XMMatrixTranspose(XMMatrixRotationQuaternion(transformComponent.Rotation));

					modelComponent.ModelMatrix = translationMatrix * rotationMatrix;
					modelComponent.IsTransformDirty = false;
				}
			}
		}

		// TODO: remove XMMATRIX viewProj arg
		void XM_CALLCONV ModelSystem::Render(CommandContext* context, XMMATRIX viewProj)
		{
			auto ecsWorld = Core::Get().GetECS();
			auto cameraSystem = ecsWorld->GetSystem<CameraSystem>();
			auto activeCamera = cameraSystem->GetActiveCamera();

			auto viewMatrix = cameraSystem->GetViewMatrix(activeCamera);
			auto projMatrix = cameraSystem->GetProjMatrix(activeCamera);


			for (const auto& entity : Entities)
			{
				auto& modelComponent = ecsWorld->GetComponent<ModelComponent>(entity);
				CameraCB cameraCB;
				cameraCB.viewProjMatrix = XMMatrixMultiply(modelComponent.ModelMatrix, viewProj);
				modelComponent.Material->SetupToRender(context);

				context->SetDynamicCBV(0, sizeof(viewProj), &viewProj);
				context->SetDynamicCBV(1, sizeof(modelComponent.ModelMatrix), &modelComponent.ModelMatrix);
				modelComponent.Mesh->Draw(context);
			}
		}
	}
}