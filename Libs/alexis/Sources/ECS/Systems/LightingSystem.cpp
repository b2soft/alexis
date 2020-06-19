#include <Precompiled.h>

#include "LightingSystem.h"

#include <ECS/ECS.h>
#include <ECS/Systems/CameraSystem.h>
#include <ECS/Systems/ShadowSystem.h>
#include <ECS/Components/TransformComponent.h>
#include <ECS/Components/LightComponent.h>

#include <Core/Core.h>
#include <Core/ResourceManager.h>
#include <Render/Render.h>
#include <Render/CommandContext.h>

#include <Render/Materials/LightingMaterial.h>

namespace alexis
{
	namespace ecs
	{
		// Todo: generalize common case
		__declspec(align(16)) struct CameraParams
		{
			XMVECTOR CameraPos;
			XMMATRIX InvViewMatrix;
			XMMATRIX InvProjMatrix;
		};

		__declspec(align(16)) struct DirectionalLight
		{
			XMVECTOR Direction;
			XMVECTOR Color; 
		};

		__declspec(align(16)) struct ShadowMapParams
		{
			XMMATRIX LightSpaceMatrix;
		};

		void LightingSystem::Init()
		{
			auto* resMgr = Core::Get().GetResourceManager();
			m_lightingMaterial = resMgr->GetBetterMaterial(L"Resources/Materials/Lighting.material");

			m_fsQuad = Core::Get().GetResourceManager()->GetMesh(L"$FS_QUAD");
		}

		void LightingSystem::Render(CommandContext* context)
		{
			auto render = Render::GetInstance();
			auto rtManager = render->GetRTManager();
			auto gbuffer = rtManager->GetRenderTarget(L"GB");
			auto shadowMapRT = rtManager->GetRenderTarget(L"Shadow Map");
			auto hdr = rtManager->GetRenderTarget(L"HDR");

			auto& depth = gbuffer->GetTexture(RenderTarget::DepthStencil);
			auto& shadowMap = shadowMapRT->GetTexture(RenderTarget::DepthStencil);

			context->TransitionResource(gbuffer->GetTexture(RenderTarget::Slot::Slot0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(gbuffer->GetTexture(RenderTarget::Slot::Slot1), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(gbuffer->GetTexture(RenderTarget::Slot::Slot2), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(depth, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(shadowMap, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			context->SetRenderTarget(*hdr);
			context->SetViewport(hdr->GetViewport());

			m_lightingMaterial->Set(context);

			auto& ecsWorld = Core::Get().GetECSWorld();
			auto cameraSystem = ecsWorld.GetSystem<CameraSystem>();
			auto activeCamera = cameraSystem->GetActiveCamera();

			auto& transformComponent = ecsWorld.GetComponent<TransformComponent>(activeCamera);

			CameraParams cameraParams;
			cameraParams.CameraPos = transformComponent.Position;
			cameraParams.InvViewMatrix = cameraSystem->GetInvViewMatrix(activeCamera);
			cameraParams.InvProjMatrix = cameraSystem->GetInvProjMatrix(activeCamera);
			context->SetDynamicCBV(0, sizeof(cameraParams), &cameraParams);

			DirectionalLight directionalLights[1];

			for (const auto& entity : Entities)
			{
				auto& lightComponent = ecsWorld.GetComponent<LightComponent>(entity);
				if (lightComponent.Type == ecs::LightComponent::LightType::Directional)
				{
					directionalLights[0].Color = lightComponent.Color;
					directionalLights[0].Direction = lightComponent.Direction;
				}
			}

			context->SetDynamicCBV(1, sizeof(directionalLights), &directionalLights);

			ShadowMapParams depthParams;
			
			auto shadowSystem = ecsWorld.GetSystem<ShadowSystem>();
			depthParams.LightSpaceMatrix = shadowSystem->GetShadowMatrix();

			context->SetDynamicCBV(2, sizeof(depthParams), &depthParams);

			m_fsQuad->Draw(context);
		}

		DirectX::XMVECTOR LightingSystem::GetSunDirection() const
		{
			for (const auto& entity : Entities)
			{
				auto& ecsWorld = Core::Get().GetECSWorld();

				auto& lightComponent = ecsWorld.GetComponent<LightComponent>(entity);
				if (lightComponent.Type == ecs::LightComponent::LightType::Directional)
				{
					return lightComponent.Direction;
				}
			}

			return { 0., 0., 0.f };
		}

	}
}
