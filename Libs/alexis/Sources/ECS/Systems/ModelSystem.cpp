#include <Precompiled.h>

#include "ModelSystem.h"

#include <Core/Core.h>
#include <Render/Render.h>
#include <Render/Mesh.h>
#include <Render/CommandContext.h>
#include <Render/Materials/MaterialBase.h>

#include <ECS/Systems/CameraSystem.h>

#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/ModelComponent.h>
#include <ECS/Components/TransformComponent.h>

namespace alexis
{
	namespace ecs
	{
		__declspec(align(16)) struct CameraCB
		{
			XMMATRIX viewMatrix;
			XMMATRIX projMatrix;
		};

		void ModelSystem::Update(float dt)
		{
			auto& ecsWorld = Core::Get().GetECSWorld();
			for (const auto& entity : Entities)
			{
				auto& modelComponent = ecsWorld.GetComponent<ModelComponent>(entity);
				auto& transformComponent = ecsWorld.GetComponent<TransformComponent>(entity);

				if (modelComponent.IsTransformDirty || transformComponent.IsTransformDirty)
				{
					XMMATRIX translationMatrix = XMMatrixTranslationFromVector(transformComponent.Position);
					XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(transformComponent.Rotation);
					XMMATRIX scalingMatrix = XMMatrixScaling(transformComponent.UniformScale, transformComponent.UniformScale, transformComponent.UniformScale);

					modelComponent.ModelMatrix = XMMatrixMultiply(XMMatrixMultiply(scalingMatrix, rotationMatrix), translationMatrix);
					
					modelComponent.IsTransformDirty = false;
					transformComponent.IsTransformDirty = false;
				}
			}
		}

		// TODO: remove XMMATRIX viewProj arg
		void XM_CALLCONV ModelSystem::Render(CommandContext* context)
		{
			PIXScopedEvent(context->List.Get(), PIX_COLOR(0, 255, 0), "ModelSystem Render");

			auto& ecsWorld = Core::Get().GetECSWorld();
			auto cameraSystem = ecsWorld.GetSystem<CameraSystem>();
			auto activeCamera = cameraSystem->GetActiveCamera();

			auto viewMatrix = cameraSystem->GetViewMatrix(activeCamera);
			auto projMatrix = cameraSystem->GetProjMatrix(activeCamera);

			//auto viewProjMatrix = XMMatrixMultiply(viewMatrix, projMatrix);

			auto* render = alexis::Render::GetInstance();
			auto* rtManager = render->GetRTManager();
			auto* gbuffer = rtManager->GetRenderTarget(L"GB");

			context->SetRenderTarget(*gbuffer);
			context->SetViewport(gbuffer->GetViewport());

			for (const auto& entity : Entities)
			{
				auto& modelComponent = ecsWorld.GetComponent<ModelComponent>(entity);
				CameraCB cameraCB;
				cameraCB.viewMatrix = viewMatrix;
				cameraCB.projMatrix = projMatrix;

				modelComponent.Material->Set(context);

				context->SetDynamicCBV(0, sizeof(cameraCB), &cameraCB);
				context->SetDynamicCBV(1, sizeof(modelComponent.ModelMatrix), &modelComponent.ModelMatrix);
				modelComponent.Mesh->Draw(context);
			}
		}
	}
}