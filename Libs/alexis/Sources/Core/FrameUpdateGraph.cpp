#include "Precompiled.h"

#include "FrameUpdateGraph.h"

#include <Core/Core.h>

#include <ECS/Systems/ModelSystem.h>
#include <ECS/Systems/ImguiSystem.h>
#include <ECS/Systems/EditorSystem.h>

namespace alexis
{
	void FrameUpdateGraph::Update(float dt)
	{
		auto& ecsWorld = Core::Get().GetECSWorld();

		ecsWorld.GetSystem<ecs::ModelSystem>()->Update(dt);
		ecsWorld.GetSystem<ecs::ImguiSystem>()->Update(dt);
		ecsWorld.GetSystem<ecs::EditorSystem>()->Update(dt);
	}
}