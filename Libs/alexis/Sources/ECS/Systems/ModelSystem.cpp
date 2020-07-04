#include <Precompiled.h>

#include "ModelSystem.h"

#include <Core/Core.h>
#include <Core/ResourceManager.h>

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

		void ModelSystem::Init()
		{
			auto* resMgr = Core::Get().GetResourceManager();
			m_zPrepassMaterial = resMgr->GetMaterial(L"Resources\\Materials\\system\\clustered_forward\\z_prepass.material");
			m_forwardPassMaterial = resMgr->GetMaterial(L"Resources\\Materials\\system\\clustered_forward\\Lighting.material");
		}

		void ModelSystem::Update(float dt)
		{
			auto& ecsWorld = Core::Get().GetECSWorld();
			for (const auto& entity : Entities)
			{
				auto& modelComponent = ecsWorld.GetComponent<ModelComponent>(entity);
				auto& transformComponent = ecsWorld.GetComponent<TransformComponent>(entity);

				if (modelComponent.IsTransformDirty)
				{
					XMMATRIX translationMatrix = XMMatrixTranslationFromVector(transformComponent.Position);
					XMMATRIX rotationMatrix = XMMatrixTranspose(XMMatrixRotationQuaternion(transformComponent.Rotation));
					XMMATRIX scalingMatrix = XMMatrixScaling(transformComponent.UniformScale, transformComponent.UniformScale, transformComponent.UniformScale);

					modelComponent.ModelMatrix = XMMatrixMultiply(translationMatrix, rotationMatrix);
					modelComponent.ModelMatrix = XMMatrixMultiply(modelComponent.ModelMatrix, scalingMatrix);

					modelComponent.IsTransformDirty = false;
				}
			}
		}

		// TODO: remove XMMATRIX viewProj arg
		void XM_CALLCONV ModelSystem::Render(CommandContext* context)
		{
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

				context->SetCBV(0, sizeof(cameraCB), &cameraCB);
				context->SetCBV(1, sizeof(modelComponent.ModelMatrix), &modelComponent.ModelMatrix);
				modelComponent.Mesh->Draw(context);
			}
		}

		__declspec(align(16)) struct CameraParams
		{
			XMMATRIX View;
			XMMATRIX Proj;
		};

		void ModelSystem::ZPrepass(CommandContext* context)
		{
			PIXScopedEvent(context->List.Get(), PIX_COLOR(0.0, 255.0, 0.0), L"Forward Z-Prepass");
			m_zPrepassMaterial->Set(context);

			auto* render = alexis::Render::GetInstance();
			auto* rtManager = render->GetRTManager();
			auto* mainRT = rtManager->GetRenderTarget(L"MainRT");

			context->TransitionResource(mainRT->GetTexture(RenderTarget::DepthStencil), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			context->ClearDSV(mainRT->GetDsv(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0);

			context->SetRenderTarget(*mainRT);
			context->SetViewport(mainRT->GetViewport());

			auto& ecsWorld = Core::Get().GetECSWorld();
			auto cameraSystem = ecsWorld.GetSystem<CameraSystem>();
			auto activeCamera = cameraSystem->GetActiveCamera();

			CameraParams cameraCB{ cameraSystem->GetViewMatrix(activeCamera), cameraSystem->GetProjMatrix(activeCamera) };
			context->SetCBV(0, sizeof(cameraCB), &cameraCB);

			for (const auto& entity : Entities)
			{
				auto& modelComponent = ecsWorld.GetComponent<ModelComponent>(entity);

				context->SetCBV(1, sizeof(modelComponent.ModelMatrix), &modelComponent.ModelMatrix);
				modelComponent.Mesh->Draw(context);
			}
		}

		void ModelSystem::ForwardPass(CommandContext* context)
		{
			PIXScopedEvent(context->List.Get(), PIX_COLOR(0, 255, 0), L"Forward Pass");
			m_forwardPassMaterial->Set(context);

			auto* render = alexis::Render::GetInstance();
			auto* rtManager = render->GetRTManager();
			auto* mainRT = rtManager->GetRenderTarget(L"MainRT");

			context->TransitionResource(mainRT->GetTexture(RenderTarget::Slot0), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
			static constexpr float k_clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			context->ClearRTV(mainRT->GetRtv(RenderTarget::Slot0), k_clearColor);

			context->TransitionResource(mainRT->GetTexture(RenderTarget::DepthStencil), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_READ);

			context->SetRenderTarget(*mainRT);
			context->SetViewport(mainRT->GetViewport());

			auto& ecsWorld = Core::Get().GetECSWorld();
			auto cameraSystem = ecsWorld.GetSystem<CameraSystem>();
			auto activeCamera = cameraSystem->GetActiveCamera();

			CameraParams cameraCB{ cameraSystem->GetViewMatrix(activeCamera), cameraSystem->GetProjMatrix(activeCamera) };
			context->SetCBV(0, sizeof(cameraCB), &cameraCB);

			for (const auto& entity : Entities)
			{
				auto& modelComponent = ecsWorld.GetComponent<ModelComponent>(entity);

				context->SetCBV(1, sizeof(modelComponent.ModelMatrix), &modelComponent.ModelMatrix);
				modelComponent.Mesh->Draw(context);
			}
		}

	}
}