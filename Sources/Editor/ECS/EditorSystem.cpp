#include <iostream>

#include "EditorSystem.h"

#include <imgui.h>

#include <Core/Core.h>
#include <Render/Render.h>
#include <Render/Mesh.h>
#include <Render/Materials/MaterialBase.h>
#include <ECS/Systems/CameraSystem.h>
#include <ECS/Components/TransformComponent.h>
#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/LightComponent.h>
#include <ECS/Components/NameComponent.h>
#include <ECS/Components/ModelComponent.h>
#include <ECS/Components/DoNotSerializeComponent.h>

#include <utils/RenderUtils.h>

namespace editor
{
	using namespace DirectX;
	using namespace alexis;

	static const float k_cameraSpeed = 10.0f;
	static const float k_cameraTurnSpeed = 0.1f;

	void EditorSystem::Init()
	{
		auto& ecsWorld = alexis::Core::Get().GetECSWorld();
		auto entity = ecsWorld.CreateEntity();
		ecsWorld.AddComponent(entity, ecs::CameraComponent{ 45.0f, 1.777778f, 0.01f, 500.f, false });
		ecsWorld.AddComponent(entity, ecs::TransformComponent{});
		ecsWorld.AddComponent(entity, ecs::NameComponent{ "Editor Camera" });
		ecsWorld.AddComponent(entity, ecs::DoNotSerializeComponent{});

		m_editorCamera = entity;

		const auto& cameraSystem = ecsWorld.GetSystem<ecs::CameraSystem>();
		cameraSystem->SetActiveCamera(m_editorCamera);
	}

	void EditorSystem::Update(float dt)
	{
		UpdateEditorCamera(dt);
		UpdateECSGUI();
	}

	alexis::ecs::Entity EditorSystem::GetActiveCamera() const
	{
		return m_editorCamera;
	}

	std::set<alexis::ecs::Entity> EditorSystem::GetEntityList() const
	{
		// Explicit copy entities
		return Entities;
	}

	bool EditorSystem::IsCameraFixed() const
	{
		return m_isCameraFixed;
	}

	void EditorSystem::ToggleFixedCamera()
	{
		m_isCameraFixed = !m_isCameraFixed;
	}

	void EditorSystem::UpdateEditorCamera(float dt)
	{
		if (!m_isCameraFixed)
		{
			m_yaw += static_cast<float>(m_deltaMouseX) * k_cameraTurnSpeed;
			m_pitch += static_cast<float>(m_deltaMouseY) * k_cameraTurnSpeed;

			m_deltaMouseX = 0;
			m_deltaMouseY = 0;

			UpdateCameraTransform(dt);
		}
	}

	void EditorSystem::UpdateECSGUI()
	{
		int i = 0;
		auto& ecsWorld = Core::Get().GetECSWorld();

		for (auto entity : Entities)
		{
			std::string name = "";
			if (ecsWorld.HasComponent<ecs::NameComponent>(entity))
			{
				auto& nameComponent = ecsWorld.GetComponent<ecs::NameComponent>(entity);
				name = nameComponent.Name;
			}

			auto entityName = (name.empty() ? "Entity " + std::to_string(i++) : name);

			if (ImGui::TreeNode(entityName.c_str()))
			{
				if (ecsWorld.HasComponent<ecs::NameComponent>(entity))
				{
					auto& nameComponent = ecsWorld.GetComponent<ecs::NameComponent>(entity);

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

				if (ecsWorld.HasComponent<ecs::TransformComponent>(entity))
				{
					auto& transformComponent = ecsWorld.GetComponent<ecs::TransformComponent>(entity);

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

				if (ecsWorld.HasComponent<ecs::CameraComponent>(entity) &&
					ImGui::TreeNode("CameraComponent"))
				{
					auto& cameraComponent = ecsWorld.GetComponent<ecs::CameraComponent>(entity);


					ImGui::TreePop();
				}

				if (ecsWorld.HasComponent<ecs::LightComponent>(entity) &&
					ImGui::TreeNode("LightComponent"))
				{
					auto& lightComponent = ecsWorld.GetComponent<ecs::LightComponent>(entity);

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

				if (ecsWorld.HasComponent<ecs::ModelComponent>(entity) &&
					ImGui::TreeNode("ModelComponent"))
				{
					auto& modelComponent = ecsWorld.GetComponent<ecs::ModelComponent>(entity);
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

	void EditorSystem::UpdateCameraTransform(float dt)
	{
		auto& ecsWorld = Core::Get().GetECSWorld();

		XMVECTOR posOffset = XMVectorSet(m_movement[3] - m_movement[2], /*up-down*/ 0.0f, m_movement[0] - m_movement[1], 1.0f) * k_cameraSpeed * static_cast<float>(dt);
		auto& cameraTransformComponent = ecsWorld.GetComponent<ecs::TransformComponent>(m_editorCamera);
		XMVECTOR newPos = cameraTransformComponent.Position + XMVector3Rotate(posOffset, cameraTransformComponent.Rotation);
		newPos = XMVectorSetW(newPos, 1.0f);

		const auto& cameraSystem = ecsWorld.GetSystem<alexis::ecs::CameraSystem>();
		cameraSystem->SetPosition(m_editorCamera, newPos);

		XMVECTOR cameraRotationQuat = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(m_pitch), XMConvertToRadians(m_yaw), 0.0f);
		cameraSystem->SetRotation(m_editorCamera, cameraRotationQuat);
	}

	void EditorSystem::OnResize(int width, int height)
	{
		float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

		auto& ecsWorld = Core::Get().GetECSWorld();
		auto cameraSystem = ecsWorld.GetSystem<alexis::ecs::CameraSystem>();

		cameraSystem->SetProjectionParams(m_editorCamera, 45.0f, aspectRatio, 0.1f, 100.0f);
	}

	void EditorSystem::OnMouseMoved(int x, int y)
	{
		m_deltaMouseX = x;
		m_deltaMouseY = y;
	}

	void EditorSystem::SetMovement(const std::array<float, 4>& movement)
	{
		m_movement = movement;
	}
}

