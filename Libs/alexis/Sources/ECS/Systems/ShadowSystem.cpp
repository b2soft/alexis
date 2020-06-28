#include <Precompiled.h>

#include "ShadowSystem.h"

#include <Core/Core.h>
#include <Core/ResourceManager.h>
#include <Render/Render.h>
#include <Render/Mesh.h>
#include <Render/CommandContext.h>

#include <ECS/Systems/CameraSystem.h>
#include <ECS/Systems/LightingSystem.h>

#include <ECS/Components/ModelComponent.h>
#include <ECS/Components/TransformComponent.h>
#include <ECS/Components/LightComponent.h>

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
			auto* resMgr = Core::Get().GetResourceManager();
			m_shadowMaterial = resMgr->GetMaterial(L"Resources/Materials/ShadowMap.material");

			auto& ecsWorld = Core::Get().GetECSWorld();
			auto entity = ecsWorld.CreateEntity();
			ecsWorld.AddComponent(entity, ecs::CameraComponent{ 0.1f, 1.0f, 0.01f, 100.f, true });
			m_phantomCamera = entity;

			auto lightingSystem = ecsWorld.GetSystem<LightingSystem>();
			XMVECTOR rotation = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(90.f), 0.f, 0);
			ecsWorld.AddComponent(entity, ecs::TransformComponent{ -lightingSystem->GetSunDirection(), rotation, {1.f} });
			
			//auto cameraSystem = ecsWorld.GetSystem<CameraSystem>();
			//cameraSystem->LookAt(m_phantomCamera, { 0.f, 0.f, 0.f }, { 0.f, 1.f, 0.f });
		}

		// TODO: remove XMMATRIX viewProj arg
		void XM_CALLCONV ShadowSystem::Render(CommandContext* context)
		{
			auto& ecsWorld = Core::Get().GetECSWorld();

			auto cameraSystem = ecsWorld.GetSystem<CameraSystem>();
			auto lightingSystem = ecsWorld.GetSystem<LightingSystem>();
			cameraSystem->SetPosition(m_phantomCamera, -lightingSystem->GetSunDirection());
			cameraSystem->LookAt(m_phantomCamera, { 0.f, 0.f, 0.f }, { 0.f, 1.f, 0.f });

			DepthCB depthParams;

			auto m2 = cameraSystem->GetViewMatrix(m_phantomCamera);
			auto proj2 = cameraSystem->GetProjMatrix(m_phantomCamera);
			depthParams.viewProjMatrix = XMMatrixMultiply(m2, proj2);

			auto* render = alexis::Render::GetInstance();
			auto* rtManager = render->GetRTManager();
			auto* shadowRT = rtManager->GetRenderTarget(L"Shadow Map");

			context->SetRenderTarget(*shadowRT);
			context->SetViewport(shadowRT->GetViewport());

			m_shadowMaterial->Set(context);

			for (const auto& entity : Entities)
			{
				auto& modelComponent = ecsWorld.GetComponent<ModelComponent>(entity);

				depthParams.modelMatrix = modelComponent.ModelMatrix;

				context->SetDynamicCBV(0, sizeof(depthParams), &depthParams);
				modelComponent.Mesh->Draw(context);
			}
		}

		DirectX::XMMATRIX ShadowSystem::GetShadowMatrix() const
		{
			auto& ecsWorld = Core::Get().GetECSWorld();

			auto cameraSystem = ecsWorld.GetSystem<CameraSystem>();

			auto modelMatrix = XMMatrixIdentity();
			auto viewMatrix = cameraSystem->GetViewMatrix(m_phantomCamera);
			auto projMatrix = cameraSystem->GetProjMatrix(m_phantomCamera);

			auto modelView = XMMatrixMultiply(modelMatrix, viewMatrix);

			return XMMatrixMultiply(modelView, projMatrix);
		}

	}
}