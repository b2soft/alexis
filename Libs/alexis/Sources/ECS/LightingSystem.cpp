#include <Precompiled.h>

#include "LightingSystem.h"

#include <ECS/ECS.h>
#include <ECS/CameraSystem.h>
#include <ECS/ShadowSystem.h>
#include <ECS/TransformComponent.h>
#include <ECS/LightComponent.h>

#include <Core/Core.h>
#include <Core/ResourceManager.h>
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
			m_lightingMaterial = std::make_unique<LightingMaterial>();

			m_fsQuad = Core::Get().GetResourceManager()->GetMesh(L"$FS_QUAD");
		}

		void LightingSystem::Render(CommandContext* context)
		{
			m_lightingMaterial->SetupToRender(context);

			auto ecsWorld = Core::Get().GetECS();
			auto cameraSystem = ecsWorld->GetSystem<CameraSystem>();
			auto activeCamera = cameraSystem->GetActiveCamera();

			auto& transformComponent = ecsWorld->GetComponent<TransformComponent>(activeCamera);

			CameraParams cameraParams;
			cameraParams.CameraPos = transformComponent.Position;
			cameraParams.InvViewMatrix = cameraSystem->GetInvViewMatrix(activeCamera);
			cameraParams.InvProjMatrix = cameraSystem->GetInvProjMatrix(activeCamera);
			context->SetDynamicCBV(0, sizeof(cameraParams), &cameraParams);

			DirectionalLight directionalLights[1];

			for (const auto& entity : Entities)
			{
				auto& lightComponent = ecsWorld->GetComponent<LightComponent>(entity);
				if (lightComponent.Type == ecs::LightComponent::LightType::Directional)
				{
					directionalLights[0].Color = lightComponent.Color;
					directionalLights[0].Direction = lightComponent.Direction;
				}
			}

			context->SetDynamicCBV(1, sizeof(directionalLights), &directionalLights);

			ShadowMapParams depthParams;
			
			auto shadowSystem = ecsWorld->GetSystem<ShadowSystem>();
			depthParams.LightSpaceMatrix = shadowSystem->GetShadowMatrix();

			context->SetDynamicCBV(2, sizeof(depthParams), &depthParams);

			m_fsQuad->Draw(context);
		}

		DirectX::XMVECTOR LightingSystem::GetSunDirection() const
		{
			for (const auto& entity : Entities)
			{
				auto ecsWorld = Core::Get().GetECS();

				auto& lightComponent = ecsWorld->GetComponent<LightComponent>(entity);
				if (lightComponent.Type == ecs::LightComponent::LightType::Directional)
				{
					return lightComponent.Direction;
				}
			}

			return { 0., 0., 0.f };
		}

	}
}
