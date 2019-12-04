#include <Precompiled.h>

#include "LightingSystem.h"

#include <ECS/ECS.h>
#include <ECS/CameraSystem.h>
#include <ECS/TransformComponent.h>

#include <Core/Core.h>
#include <Core/ResourceManager.h>
#include <Render/CommandContext.h>

#include <Render/Materials/LightingMaterial.h>

namespace alexis
{
	namespace ecs
	{

		struct SunLight
		{
			XMFLOAT4 Parameters;
			XMVECTOR ViewPos;
		};

		// Todo: generalize common case
		__declspec(align(16)) struct CameraParams
		{
			XMMATRIX invViewMatrix;
			XMMATRIX invProjMatrix;
		};

		static SunLight s_sunLight;

		void LightingSystem::Init()
		{
			m_lightingMaterial = std::make_unique<LightingMaterial>();

			m_fsQuad = Core::Get().GetResourceManager()->GetMesh(L"$FS_QUAD");

			//s_sunLight.Parameters = XMFLOAT4{ -0.7f, -1.f, 0.f, 1.f };
			s_sunLight.Parameters = XMFLOAT4{ -0.f, -1.f, 0.f, 1.f };
		}

		void LightingSystem::Render(CommandContext* context)
		{
			m_lightingMaterial->SetupToRender(context);

			auto ecsWorld = Core::Get().GetECS();
			auto cameraSystem = ecsWorld->GetSystem<CameraSystem>();
			auto activeCamera = cameraSystem->GetActiveCamera();

			auto& transformComponent = ecsWorld->GetComponent<TransformComponent>(activeCamera);
			s_sunLight.ViewPos = transformComponent.Position;
			context->SetDynamicCBV(1, sizeof(s_sunLight), &s_sunLight);

			CameraParams cameraParams;
			cameraParams.invViewMatrix = cameraSystem->GetInvViewMatrix(activeCamera);
			cameraParams.invProjMatrix = cameraSystem->GetInvProjMatrix(activeCamera);
			context->SetDynamicCBV(0, sizeof(cameraParams), &cameraParams);

			m_fsQuad->Draw(context);

			//auto ecsWorld = Core::Get().GetECS();

			//for (const auto& entity : Entities)
			//{
			//	auto& modelComponent = ecsWorld->GetComponent<ModelComponent>(entity);

			//	// Update the MVP matrix
			//	XMMATRIX mvpMatrix = XMMatrixMultiply(modelComponent.ModelMatrix, viewProj);

			//	modelComponent.Material->SetupToRender(context);

			//	context->SetDynamicCBV(0, sizeof(mvpMatrix), &mvpMatrix);
			//	modelComponent.Mesh->Draw(context);

			//}
		}
	}
}
