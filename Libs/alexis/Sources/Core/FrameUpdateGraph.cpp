#include "Precompiled.h"

#include "FrameUpdateGraph.h"

#include <Core/Core.h>

#include <ECS/ModelSystem.h>
#include <ECS/ImguiSystem.h>

namespace alexis
{
	void FrameUpdateGraph::Update(float dt)
	{
		auto ecsWorld = Core::Get().GetECS();

		ecsWorld->GetSystem<ecs::ModelSystem>()->Update(dt);
		ecsWorld->GetSystem<ecs::ImguiSystem>()->Update(dt);
	}
}