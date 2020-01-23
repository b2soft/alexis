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

			auto projMatrix = XMMatrixOrthographicLH(-10.f, 10.f, 0.001f, 100.f);

			auto lightingSystem = ecsWorld->GetSystem<LightingSystem>();
			auto viewMatrix = XMMatrixLookAtLH(lightingSystem->GetSunDirection(), { 0.f, 0.f, 0.f }, { 0.f, 1.f, 0.f });

			m_shadowMaterial->SetupToRender(context);

			for (const auto& entity : Entities)
			{
				auto depthMVP = XMMatrixMultiply(viewMatrix, projMatrix);

				auto& modelComponent = ecsWorld->GetComponent<ModelComponent>(entity);

				DepthCB depthCB;
				depthCB.viewProjMatrix = depthMVP;

				context->SetDynamicCBV(0, sizeof(depthCB), &depthCB);
				modelComponent.Mesh->Draw(context);
			}
		}
	}
}