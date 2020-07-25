#include "Precompiled.h"

#include <iostream>
#include "EditorSystem.h"

#include <imgui.h>
#include <numbers>


#include <Core/Core.h>
#include <Render/Mesh.h>
#include <Render/Materials/MaterialBase.h>
#include <ECS/Components/TransformComponent.h>
#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/LightComponent.h>
#include <ECS/Components/NameComponent.h>
#include <ECS/Components/ModelComponent.h>
#include <ECS/Components/DoNotSerializeComponent.h>

#include <ecs/Systems/EditorSystemSerializer.h>

#include <utils/RenderUtils.h>

namespace alexis
{
	void ecs::EditorSystem::LoadScene(std::wstring_view path)
	{
		UnloadScene();
		EditorSystemSerializer::Load(std::wstring(path));
		m_loadedScene = { std::wstring(path) };
	}

	void ecs::EditorSystem::UnloadScene()
	{
		if (!m_loadedScene.has_value())
		{
			return;
		}

		auto& ecsWorld = Core::Get().GetECSWorld();
		auto entities = Entities; // clone entity ids
		for (auto entity : entities)
		{
			if (ecsWorld.HasComponent<DoNotSerializeComponent>(entity))
			{
				continue;
			}

			ecsWorld.DestroyEntity(entity);
		}
	}

	void ecs::EditorSystem::SaveScene()
	{
		EditorSystemSerializer::Save(m_loadedScene->Path);
	}

	void ecs::EditorSystem::Update(float dt)
	{
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("Editor"))
			{
				if (ImGui::BeginMenu("Open"))
				{
					std::string path = "Resources\\Scenes"; // TODO: Remove hardcode later
					for (const auto& filePath : std::filesystem::directory_iterator(path))
					{
						std::wstring pathWStr = filePath.path();
						std::string pathStr{ pathWStr.cbegin(), pathWStr.cend() };
						if (ImGui::MenuItem(pathStr.c_str()))
						{
							LoadScene(pathWStr);
						}
					}

					ImGui::EndMenu();
				}

				if (ImGui::MenuItem("Save", "Ctrl+S"))
				{
					SaveScene();
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		int i = 0;
		auto& ecsWorld = Core::Get().GetECSWorld();

		for (auto entity : Entities)
		{
			std::string name = "";
			if (ecsWorld.HasComponent<NameComponent>(entity))
			{
				auto& nameComponent = ecsWorld.GetComponent<NameComponent>(entity);
				name = nameComponent.Name;
			}

			auto entityName = (name.empty() ? "Entity " + std::to_string(i++) : name);

			if (ImGui::TreeNode(entityName.c_str()))
			{
				if (ecsWorld.HasComponent<NameComponent>(entity))
				{
					auto& nameComponent = ecsWorld.GetComponent<NameComponent>(entity);

					if (ImGui::TreeNode("NameComponent"))
					{
						char buf[128] = "";
						strcpy_s(buf, nameComponent.Name.c_str());
						if (ImGui::InputText("Name", buf, 128, ImGuiInputTextFlags_EnterReturnsTrue))
						{
							nameComponent.Name = buf;
						}

						ImGui::TreePop();
					}

					name = nameComponent.Name;
				}

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

						XMFLOAT3 pyr = utils::GetPitchYawRollFromQuaternion(transformComponent.Rotation);
						float rotationGUI[3]{ XMConvertToDegrees(pyr.x), XMConvertToDegrees(pyr.y), XMConvertToDegrees(pyr.z) };
						if (ImGui::InputFloat3("Rotation", rotationGUI, 3))
						{
							XMVECTOR pyrVec{ XMConvertToRadians(rotationGUI[0]), XMConvertToRadians(rotationGUI[1]), XMConvertToRadians(rotationGUI[2]) };
							transformComponent.Rotation = XMQuaternionRotationRollPitchYawFromVector(pyrVec);

							transformComponent.IsTransformDirty = true;
						}

						float scaleGUI = transformComponent.UniformScale;
						if (ImGui::InputFloat("Scale", &scaleGUI))
						{
							transformComponent.UniformScale = scaleGUI;
							transformComponent.IsTransformDirty = true;
						}

						ImGui::TreePop();
					}
				}

				if (ecsWorld.HasComponent<CameraComponent>(entity) &&
					ImGui::TreeNode("CameraComponent"))
				{
					auto& cameraComponent = ecsWorld.GetComponent<CameraComponent>(entity);


					ImGui::TreePop();
				}

				if (ecsWorld.HasComponent<LightComponent>(entity) &&
					ImGui::TreeNode("LightComponent"))
				{
					auto& lightComponent = ecsWorld.GetComponent<LightComponent>(entity);

					ImGui::Text("Type: Point");

					XMFLOAT4 lightColor{};
					XMStoreFloat4(&lightColor, lightComponent.Color);

					float lightColorGUI[3]{ lightColor.x, lightColor.y, lightColor.z };
					if (ImGui::InputFloat3("Color", lightColorGUI, 3))
					{
						lightComponent.Color = { lightColorGUI[0], lightColorGUI[1], lightColorGUI[2] };
					}

					ImGui::TreePop();
				}

				if (ecsWorld.HasComponent<ModelComponent>(entity) &&
					ImGui::TreeNode("ModelComponent"))
				{
					auto& modelComponent = ecsWorld.GetComponent<ModelComponent>(entity);
					const auto& meshPath = modelComponent.Mesh->GetPath();
					std::string meshPathStr{ meshPath.cbegin(), meshPath.cend() };
					ImGui::Text("Mesh: %s", meshPathStr.c_str());

					const auto& materialPath = modelComponent.Material->GetPath();
					std::string materialPathStr{ materialPath.cbegin(), materialPath.cend() };
					ImGui::Text("Material: %s", materialPathStr.c_str());

					ImGui::TreePop();
				}

				ImGui::TreePop();
			}
		}


	}
}

