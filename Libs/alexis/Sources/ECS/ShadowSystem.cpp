#include <Precompiled.h>

#include "ShadowSystem.h"

#include <Core/Core.h>
#include <Render/Mesh.h>
#include <Render/CommandContext.h>
#include <Render/Materials/ShadowMaterial.h>

#include <ECS/CameraSystem.h>
#include <ECS/LightingSystem.h>

#include <ECS/CameraComponent.h>
#include <ECS/ModelComponent.h>
#include <ECS/TransformComponent.h>
#include <ECS/LightComponent.h>

namespace alexis
{
	namespace ecs
	{
		__declspec(align(16)) struct DepthCB
		{
			XMMATRIX modelMatrix;
			XMMATRIX viewProjMatrix;
		};

		void ShadowSystem::Init()
		{
			m_shadowMaterial = std::make_unique<ShadowMaterial>();
		}

		// TODO: remove XMMATRIX viewProj arg
		void XM_CALLCONV ShadowSystem::Render(CommandContext* context)
		{
			auto ecsWorld = Core::Get().GetECS();

			//auto projMatrix = XMMatrixOrthographicLH(-10.f, 10.f, 0.001f, 100.f);
			//
			//auto lightingSystem = ecsWorld->GetSystem<LightingSystem>();
			//auto viewMatrix = XMMatrixLookAtLH({ 0.f, 16.f, 0.f }, { 0.f, 0.f, 0.f }, { 0.f, 1.f, 0.f });
			//
			//auto depthMVP = XMMatrixMultiply(viewMatrix, projMatrix);

			m_shadowMaterial->SetupToRender(context);


			auto cameraSystem = ecsWorld->GetSystem<CameraSystem>();
			auto activeCamera = cameraSystem->GetActiveCamera();

			auto& transformComponent = ecsWorld->GetComponent<TransformComponent>(activeCamera);

			auto viewMatrix = cameraSystem->GetViewMatrix(activeCamera);
			auto projMatrix = cameraSystem->GetProjMatrix(activeCamera);

			auto viewProjMatrix = XMMatrixMultiply(viewMatrix, projMatrix);

			DepthCB depthCB;
			depthCB.viewProjMatrix = viewProjMatrix;

			

			for (const auto& entity : Entities)
			{
				auto& modelComponent = ecsWorld->GetComponent<ModelComponent>(entity);

				depthCB.modelMatrix = modelComponent.ModelMatrix;

				context->SetDynamicCBV(0, sizeof(depthCB), &depthCB);
				modelComponent.Mesh->Draw(context);
			}
		}
	}
}