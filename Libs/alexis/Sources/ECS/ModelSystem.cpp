#include <Precompiled.h>

#include "ModelSystem.h"

#include <Core/Core.h>
#include <Render/Mesh.h>
#include <Render/CommandContext.h>
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

		void ModelSystem::Render(CommandContext* context, XMMATRIX view/*, XMMATRIX proj*/)
		{
			auto ecsWorld = Core::Get().GetECS();
			for (const auto& entity : Entities)
			{
				auto& modelComponent = ecsWorld->GetComponent<ModelComponent>(entity);

				// Update the MVP matrix
				XMMATRIX mvpMatrix = XMMatrixMultiply(modelComponent.ModelMatrix, view);
				//mvpMatrix = XMMatrixMultiply(mvpMatrix, proj);

				context->SetDynamicCBV(0, modelComponent.CBV);

				ModelComponent::Mat matrices;
				matrices.ModelViewProjectionMatrix = mvpMatrix;
				memcpy(modelComponent.CBV.GetCPUPtr() , &matrices, sizeof(ModelComponent::Mat));

				modelComponent.Mesh->Draw(context);
			}
		}
	}
}