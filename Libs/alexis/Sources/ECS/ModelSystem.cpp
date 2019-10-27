#include <Precompiled.h>

#include "ModelSystem.h"

#include <Core/Core.h>
#include <Render/Mesh.h>
#include <Render/CommandContext.h>

namespace alexis
{
	namespace ecs
	{

		void ModelSystem::Render(CommandContext* context)
		{
			auto ecsWorld = Core::Get().GetECS();
			for (const auto& entity : Entities)
			{
				auto& modelComponent = ecsWorld->GetComponent<ModelComponent>(entity);
				auto& transformcOomponent = ecsWorld->GetComponent<TransformComponent>(entity);

				modelComponent.Mesh->Draw(context);
			}
		}
	}
}