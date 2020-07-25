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

		__declspec(align(16)) struct ScreenParams
		{
			XMVECTOR Params;
		};

		__declspec(align(16)) struct PointLightParams
		{
			XMVECTOR Position;
			XMVECTOR Color;
		};

		float CalculatePointLightScale(const XMVECTOR& color, const XMVECTOR& position)
		{
			XMFLOAT4 colorOut;
			XMStoreFloat4(&colorOut, color);
			float maxChannel = std::fmax(std::fmax(colorOut.x, colorOut.y), colorOut.z);

			return sqrtf(maxChannel * 256);
		}

		void LightingSystem::Init()
		{
			auto* resMgr = Core::Get().GetResourceManager();

			m_fsQuad = Core::Get().GetResourceManager()->GetMesh(L"$FS_QUAD");
			m_sphere = Core::Get().GetResourceManager()->GetMesh(L"Resources/Models/Sphere.dae");

			// Point Light Stencil Material
			{
				MaterialLoadParams params;
				params.VSPath = L"PointLight_vs";
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
				m_pointLightStencil = std::make_unique<Material>(params);
			}

			// Point Light Material
			{
				MaterialLoadParams params;
				params.VSPath = L"PointLight_vs";
				params.PSPath = L"PointLight_ps";
				params.Textures = { L"$GB#0", L"$GB#1", L"$GB#2", L"$GB#Depth", L"$Shadow Map#Depth" };
				params.RTV = L"$HDR";

				params.CullMode = D3D12_CULL_MODE_FRONT;

				CD3DX12_BLEND_DESC blendDesc{ D3D12_DEFAULT };
				blendDesc.RenderTarget[0].BlendEnable = true;
				blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
				blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;

				params.BlendDesc = blendDesc;

				CD3DX12_DEPTH_STENCIL_DESC depthDesc{ D3D12_DEFAULT };
				depthDesc.DepthEnable = false;
				depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
				depthDesc.StencilEnable = TRUE;
				depthDesc.StencilWriteMask = 0x00;
				depthDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NOT_EQUAL;
				depthDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NOT_EQUAL;

				params.CustomDS = true;
				params.DepthStencil = depthDesc;
				m_pointLight = std::make_unique<Material>(params);
			}

			// Ambient Light Material
			{
				MaterialLoadParams params;
				params.VSPath = L"AmbientLight_vs";
				params.PSPath = L"AmbientLight_ps";
				//params.Textures = { L"$GB#0", L"$GB#1", L"$GB#2", L"$GB#Depth", L"$Shadow Map#Depth", L"$CUBEMAP_Irradiance", L"$CUBEMAP_Prefiltered", L"$ConvolutedBRDF" };
				params.Textures = { L"$GB#0", L"$GB#1", L"$GB#2", L"$GB#Depth", L"$CUBEMAP_Irradiance", L"$CUBEMAP_Prefiltered", L"$ConvolutedBRDF" };
				params.RTV = L"$HDR";

				CD3DX12_BLEND_DESC blendDesc{ D3D12_DEFAULT };
				blendDesc.RenderTarget[0].BlendEnable = true;
				blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
				blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;

				params.BlendDesc = blendDesc;

				params.DepthEnable = false;
				m_ambientLight = std::make_unique<Material>(params);
			}
		}

		void LightingSystem::Render(CommandContext* context)
		{
			PointLights(context);
			AmbientLight(context);
		}

		void LightingSystem::PointLights(CommandContext* context)
		{
			auto* render = Render::GetInstance();
			auto* rtManager = render->GetRTManager();
			auto* gbuffer = rtManager->GetRenderTarget(L"GB");
			auto* hdr = rtManager->GetRenderTarget(L"HDR");
			auto& depth = gbuffer->GetTexture(RenderTarget::DepthStencil);

			context->TransitionResource(gbuffer->GetTexture(RenderTarget::Slot::Slot0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(gbuffer->GetTexture(RenderTarget::Slot::Slot1), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(gbuffer->GetTexture(RenderTarget::Slot::Slot2), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(gbuffer->GetTexture(RenderTarget::Slot::DepthStencil), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);

			context->SetRenderTarget(*hdr, *gbuffer);
			context->SetViewport(hdr->GetViewport());

			auto& ecsWorld = Core::Get().GetECSWorld();
			auto cameraSystem = ecsWorld.GetSystem<CameraSystem>();
			auto activeCamera = cameraSystem->GetActiveCamera();
			auto& transformComponent = ecsWorld.GetComponent<TransformComponent>(activeCamera);

			// Point Lights
			{
				PIXScopedEvent(context->List.Get(), PIX_COLOR(0, 255, 0), "Point Light Stencil");

				m_pointLightStencil->Set(context);

				for (auto entity : Entities)
				{
					auto& lightComponent = ecsWorld.GetComponent<LightComponent>(entity);
					auto& transformComponent = ecsWorld.GetComponent<TransformComponent>(entity);
					if (lightComponent.Type == ecs::LightComponent::LightType::Point)
					{
						float scale = CalculatePointLightScale(lightComponent.Color, transformComponent.Position);
						XMMATRIX wvpMatrix = XMMatrixIdentity() * XMMatrixScaling(scale, scale, scale) * XMMatrixTranslationFromVector(transformComponent.Position) * cameraSystem->GetViewMatrix(activeCamera) * cameraSystem->GetProjMatrix(activeCamera);

						LightParams lightCB{ wvpMatrix };
						context->SetDynamicCBV(0, sizeof(lightCB), &lightCB);

						m_sphere->Draw(context);
					}
				}
			}

			{
				PIXScopedEvent(context->List.Get(), PIX_COLOR(0, 255, 0), "Point Light Shading");
				auto barrierStencil = CD3DX12_RESOURCE_BARRIER::Transition(depth.GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0);
				auto barrierStencil2 = CD3DX12_RESOURCE_BARRIER::Transition(depth.GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_READ, 1);
				D3D12_RESOURCE_BARRIER barriers[] = { barrierStencil , barrierStencil2 };
				context->List->ResourceBarrier(2, barriers);
				context->List->OMSetStencilRef(0);

				m_pointLight->Set(context);

				ScreenParams screenParams;
				screenParams.Params = { static_cast<float>(alexis::g_clientWidth), static_cast<float>(alexis::g_clientHeight),
					1.0f / alexis::g_clientWidth, 1.0f / alexis::g_clientHeight };

				context->SetDynamicCBV(2, sizeof(screenParams), &screenParams);

				CameraParams cameraParams;
				cameraParams.CameraPos = transformComponent.Position;
				cameraParams.InvViewMatrix = cameraSystem->GetInvViewMatrix(activeCamera);
				cameraParams.InvProjMatrix = cameraSystem->GetInvProjMatrix(activeCamera);

				context->SetDynamicCBV(3, sizeof(cameraParams), &cameraParams);

				for (auto entity : Entities)
				{
					auto& lightComponent = ecsWorld.GetComponent<LightComponent>(entity);
					auto& transformComponent = ecsWorld.GetComponent<TransformComponent>(entity);
					if (lightComponent.Type == ecs::LightComponent::LightType::Point)
					{
						float scale = CalculatePointLightScale(lightComponent.Color, transformComponent.Position);
						XMMATRIX wvpMatrix = XMMatrixIdentity() * XMMatrixScaling(scale, scale, scale) * XMMatrixTranslationFromVector(transformComponent.Position) * cameraSystem->GetViewMatrix(activeCamera) * cameraSystem->GetProjMatrix(activeCamera);

						PointLightParams pl{ transformComponent.Position, lightComponent.Color };

						context->SetDynamicCBV(1, sizeof(pl), &pl);

						m_sphere->Draw(context);
					}
				}
			}
		}

		void LightingSystem::AmbientLight(CommandContext* context)
		{
			auto* render = Render::GetInstance();
			auto* rtManager = render->GetRTManager();
			auto* gbuffer = rtManager->GetRenderTarget(L"GB");
			//auto* shadowMapRT = rtManager->GetRenderTarget(L"Shadow Map");
			auto* hdr = rtManager->GetRenderTarget(L"HDR");
			auto* irradiance = rtManager->GetRenderTarget(L"CUBEMAP_Irradiance");
			auto* prefiltered = rtManager->GetRenderTarget(L"CUBEMAP_Prefiltered");
			auto* convBRDF = rtManager->GetRenderTarget(L"ConvolutedBRDF");

			auto& depth = gbuffer->GetTexture(RenderTarget::DepthStencil);
			//auto& shadowMap = shadowMapRT->GetTexture(RenderTarget::DepthStencil);

			context->SetRenderTarget(*hdr, *gbuffer);
			context->SetViewport(hdr->GetViewport());

			m_ambientLight->Set(context);

			auto& ecsWorld = Core::Get().GetECSWorld();
			auto cameraSystem = ecsWorld.GetSystem<CameraSystem>();
			auto activeCamera = cameraSystem->GetActiveCamera();

			auto& transformComponent = ecsWorld.GetComponent<TransformComponent>(activeCamera);

			CameraParams cameraParams;
			cameraParams.CameraPos = transformComponent.Position;
			cameraParams.InvViewMatrix = cameraSystem->GetInvViewMatrix(activeCamera);
			cameraParams.InvProjMatrix = cameraSystem->GetInvProjMatrix(activeCamera);
			context->SetDynamicCBV(0, sizeof(cameraParams), &cameraParams);

			//ShadowMapParams depthParams;

			//auto shadowSystem = ecsWorld.GetSystem<ShadowSystem>();
			//depthParams.LightSpaceMatrix = shadowSystem->GetShadowMatrix();

			//context->SetDynamicCBV(2, sizeof(depthParams), &depthParams);

			m_fsQuad->Draw(context);
		}

		DirectX::XMVECTOR LightingSystem::GetSunDirection() const
		{
			return { 0., 0., 0.f };
		}

	}
}
