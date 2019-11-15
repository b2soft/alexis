#include <Precompiled.h>

#include "ModelSystem.h"

#include <Core/Core.h>
#include <Render/Mesh.h>
#include <Render/CommandContext.h>
#include <Render/Materials/MaterialBase.h>

#include <ECS/ModelComponent.h>
#include <ECS/TransformComponent.h>

namespace alexis
{
	namespace ecs
	{
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
		void ModelSystem::Render(CommandContext* context, XMMATRIX viewProj)
		{
			auto ecsWorld = Core::Get().GetECS();
			for (const auto& entity : Entities)
			{
				auto& modelComponent = ecsWorld->GetComponent<ModelComponent>(entity);

				// Update the MVP matrix
				XMMATRIX mvpMatrix = XMMatrixMultiply(modelComponent.ModelMatrix, viewProj);

				modelComponent.Material->SetupToRender(context);

				context->SetDynamicCBV(0, sizeof(mvpMatrix), &mvpMatrix);
				modelComponent.Mesh->Draw(context);
			}
		}
	}
}