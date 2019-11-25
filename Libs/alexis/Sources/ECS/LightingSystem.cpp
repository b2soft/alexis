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

		static SunLight s_sunLight;

		void LightingSystem::Init()
		{
			m_lightingMaterial = std::make_unique<LightingMaterial>();

			m_fsQuad = Core::Get().GetResourceManager()->GetMesh(L"$FS_QUAD");

			s_sunLight.Parameters = XMFLOAT4{ 0.0f, -1.0f, 0.0f, 700.f };
		}

		void LightingSystem::Render(CommandContext* context)
		{
			m_lightingMaterial->SetupToRender(context);

			auto ecsWorld = Core::Get().GetECS();
			auto cameraSystem = ecsWorld->GetSystem<CameraSystem>();
			auto& transformComponent = ecsWorld->GetComponent<TransformComponent>(cameraSystem->GetActiveCamera());
			s_sunLight.ViewPos = transformComponent.Position;

			context->SetDynamicCBV(0, sizeof(s_sunLight), &s_sunLight);

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
