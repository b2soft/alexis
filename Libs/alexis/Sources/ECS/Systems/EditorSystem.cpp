#include "Precompiled.h"

#include "EditorSystem.h"

#include <imgui.h>
#include <numbers>

#include <Core/Core.h>
#include <ECS/Components/TransformComponent.h>

namespace alexis
{
	void ecs::EditorSystem::Update(float dt)
	{
		if (ImGui::TreeNode("Entities"))
		{
			int i = 0;
			auto& ecsWorld = Core::Get().GetECSWorld();

			for (auto entity : Entities)
			{
				if (ImGui::TreeNode(("Entity " + std::to_string(i++)).c_str()))
				{
					if (ecsWorld.HasComponent<TransformComponent>(entity))
					{
						auto& transformComponent = ecsWorld.GetComponent<TransformComponent>(entity);

						if (ImGui::TreeNode("TransformComponent"))
						{
							XMFLOAT4 position{};
							XMStoreFloat4(&position, transformComponent.Position);

							float positionGUI[3]{ position.x, position.y, position.z };
							if (ImGui::InputFloat3("Position", positionGUI, 3))
							{
								XMFLOAT4 positionDX(positionGUI);
								transformComponent.Position = XMLoadFloat4(&positionDX);

								transformComponent.IsTransformDirty = true;
							}

							ImGui::TreePop();
						}
					}

					ImGui::TreePop();
				}
			}

			ImGui::TreePop();
		}

	}
}

