#include <Precompiled.h>

#include "LightingSystem.h"

#include <Core/Core.h>
#include <Core/ResourceManager.h>
#include <Render/CommandContext.h>

#include <Render/Materials/LightingMaterial.h>

namespace alexis
{
	namespace ecs
	{

		void LightingSystem::Init()
		{
			m_lightingMaterial = std::make_unique<LightingMaterial>();

			m_fsQuad = Core::Get().GetResourceManager()->GetMesh(L"$FS_QUAD");
		}

		void LightingSystem::Render(CommandContext* context)
		{
			m_lightingMaterial->SetupToRender(context);

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
