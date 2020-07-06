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

		__declspec(align(16)) struct LightParams
		{
			XMMATRIX WVPMatriX;
		};

		__declspec(align(16)) struct PointLight
		{
			XMVECTOR Position;
			XMVECTOR Color;
			struct
			{
				float Linear;
				float Exp;
			} Attenuation;
		};

		float CalculatePointLightScale(const PointLight& light)
		{
			XMFLOAT4 color;
			XMStoreFloat4(&color, light.Color);
			float maxChannel = std::fmax(std::fmax(color.x, color.y), color.z);

			float ret = (light.Attenuation.Linear + sqrtf(light.Attenuation.Linear * light.Attenuation.Linear - 4 * light.Attenuation.Exp * (light.Attenuation.Exp - 256 * maxChannel))) /
				(2 * light.Attenuation.Exp);

			return ret;
		}

		void LightingSystem::Init()
		{
			auto* resMgr = Core::Get().GetResourceManager();
			m_lightingMaterial = resMgr->GetMaterial(L"Resources/Materials/system/Lighting.material");

			m_fsQuad = Core::Get().GetResourceManager()->GetMesh(L"$FS_QUAD");
			m_sphere = Core::Get().GetResourceManager()->GetMesh(L"Resources/Models/Sphere.dae");

			{
				MaterialLoadParams params;
				params.VSPath = L"Lighting_vs";
				//params.Textures = { L"$GB#0", L"$GB#1", L"$GB#2", L"$GB#Depth", L"$Shadow Map#Depth", L"$CUBEMAP_Irradiance", L"$CUBEMAP_Prefiltered", L"$ConvolutedBRDF" };
				params.RTV = L"$HDR";

				params.CullMode = D3D12_CULL_MODE_NONE;

				CD3DX12_DEPTH_STENCIL_DESC depthDesc{ D3D12_DEFAULT };
				depthDesc.DepthEnable = TRUE;
				depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
				depthDesc.StencilEnable = TRUE;
				depthDesc.StencilReadMask = 0x00;
				depthDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
				depthDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_INCR;
				depthDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
				depthDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
				depthDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_DECR;
				depthDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;

				params.CustomDS = true;
				params.DepthStencil = depthDesc;
				m_lightStencil = std::make_unique<Material>(params);
			}

			{
				MaterialLoadParams params;
				params.VSPath = L"Lighting_vs";
				params.PSPath = L"Lighting_ps";
				params.Textures = { L"$GB#0", L"$GB#1", L"$GB#2", L"$GB#Depth", L"$Shadow Map#Depth", L"$CUBEMAP_Irradiance", L"$CUBEMAP_Prefiltered", L"$ConvolutedBRDF" };
				params.RTV = L"$HDR";

				params.CullMode = D3D12_CULL_MODE_FRONT;

				CD3DX12_DEPTH_STENCIL_DESC depthDesc{ D3D12_DEFAULT };
				depthDesc.DepthEnable = false;
				depthDesc.StencilEnable = TRUE;
				depthDesc.StencilWriteMask = 0x00;
				depthDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NOT_EQUAL;
				depthDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NOT_EQUAL;

				params.CustomDS = true;
				params.DepthStencil = depthDesc;
				m_lightShading = std::make_unique<Material>(params);
			}
		}

		void LightingSystem::Render(CommandContext* context)
		{
			auto* render = Render::GetInstance();
			auto* rtManager = render->GetRTManager();
			auto* gbuffer = rtManager->GetRenderTarget(L"GB");
			auto* shadowMapRT = rtManager->GetRenderTarget(L"Shadow Map");
			auto* hdr = rtManager->GetRenderTarget(L"HDR");
			auto* irradiance = rtManager->GetRenderTarget(L"CUBEMAP_Irradiance");
			auto* prefiltered = rtManager->GetRenderTarget(L"CUBEMAP_Prefiltered");
			auto* convBRDF = rtManager->GetRenderTarget(L"ConvolutedBRDF");

			auto& depth = gbuffer->GetTexture(RenderTarget::DepthStencil);
			auto& shadowMap = shadowMapRT->GetTexture(RenderTarget::DepthStencil);

			context->TransitionResource(gbuffer->GetTexture(RenderTarget::Slot::Slot0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(gbuffer->GetTexture(RenderTarget::Slot::Slot1), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(gbuffer->GetTexture(RenderTarget::Slot::Slot2), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(depth, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(shadowMap, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(irradiance->GetTexture(RenderTarget::Slot::Slot0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(prefiltered->GetTexture(RenderTarget::Slot::Slot0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(convBRDF->GetTexture(RenderTarget::Slot::Slot0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			context->SetRenderTarget(*hdr, *gbuffer);
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

		void LightingSystem::Render2(CommandContext* context)
		{
			auto* render = Render::GetInstance();
			auto* rtManager = render->GetRTManager();
			auto* gbuffer = rtManager->GetRenderTarget(L"GB");
			auto* shadowMapRT = rtManager->GetRenderTarget(L"Shadow Map");
			auto* hdr = rtManager->GetRenderTarget(L"HDR");
			auto* irradiance = rtManager->GetRenderTarget(L"CUBEMAP_Irradiance");
			auto* prefiltered = rtManager->GetRenderTarget(L"CUBEMAP_Prefiltered");
			auto* convBRDF = rtManager->GetRenderTarget(L"ConvolutedBRDF");

			auto& depth = gbuffer->GetTexture(RenderTarget::DepthStencil);
			auto& shadowMap = shadowMapRT->GetTexture(RenderTarget::DepthStencil);

			context->TransitionResource(gbuffer->GetTexture(RenderTarget::Slot::Slot0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(gbuffer->GetTexture(RenderTarget::Slot::Slot1), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(gbuffer->GetTexture(RenderTarget::Slot::Slot2), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			auto barrierStencil = CD3DX12_RESOURCE_BARRIER::Transition(depth.GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			context->List->ResourceBarrier(1, &barrierStencil);

			context->TransitionResource(shadowMap, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(irradiance->GetTexture(RenderTarget::Slot::Slot0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(prefiltered->GetTexture(RenderTarget::Slot::Slot0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(convBRDF->GetTexture(RenderTarget::Slot::Slot0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			context->SetRenderTarget(*hdr, *gbuffer);
			context->SetViewport(hdr->GetViewport());

			m_lightStencil->Set(context);

			auto& ecsWorld = Core::Get().GetECSWorld();
			auto cameraSystem = ecsWorld.GetSystem<CameraSystem>();
			auto activeCamera = cameraSystem->GetActiveCamera();

			auto& transformComponent = ecsWorld.GetComponent<TransformComponent>(activeCamera);

			PointLight pl;
			pl.Color = { 1.0, 0.0, 0.0, 1.0 };
			pl.Position = { 0.0, 0.0, 0.0, 0.0 };
			pl.Attenuation.Exp = 1.0;
			pl.Attenuation.Linear = 1.0;

			float scale = CalculatePointLightScale(pl);
			XMMATRIX wvpMatrix = XMMatrixIdentity()* XMMatrixScaling(scale, scale, scale) * XMMatrixTranslationFromVector(pl.Position) * cameraSystem->GetViewMatrix(activeCamera) * cameraSystem->GetProjMatrix(activeCamera);

			LightParams lightCB{ wvpMatrix };
			context->SetDynamicCBV(3, sizeof(lightCB), &lightCB);
			context->SetDynamicCBV(4, sizeof(pl), &pl);

			{
				PIXScopedEvent(context->List.Get(), PIX_COLOR(0, 255, 0), "Point Light Stencil");
				m_sphere->Draw(context);
			}

			{
				PIXScopedEvent(context->List.Get(), PIX_COLOR(0, 255, 0), "Point Light Shading");
				auto barrierStencil2 = CD3DX12_RESOURCE_BARRIER::Transition(depth.GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0);
				auto barrierStencil = CD3DX12_RESOURCE_BARRIER::Transition(depth.GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_READ, 1);
				D3D12_RESOURCE_BARRIER barriers[] = { barrierStencil , barrierStencil2 };
				context->List->ResourceBarrier(2, barriers);

				m_lightShading->Set(context);

				context->List->OMSetStencilRef(0);

				m_sphere->Draw(context);
			}
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
