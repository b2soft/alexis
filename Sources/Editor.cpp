#include "Precompiled.h"

#include "Editor.h"

#include <ECS/ECS.h>
#include <ECS/Systems/EditorSystem.h>

void EditorApp::OnResize(int width, int height)
{
	using namespace alexis;
	auto& ecsWorld = Core::Get().GetECSWorld();
	const auto& editorSystem = ecsWorld.GetSystem<ecs::EditorSystem>();

	editorSystem->OnResize(width, height);
}
