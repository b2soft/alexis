#include "Precompiled.h"

#include "EditorSystemSerializer.h"

#include <json.hpp>
#include <fstream>

#include <Core/Core.h>
#include <Core/ResourceManager.h>

#include <ECS/ECS.h>
#include <ECS/Systems/EditorSystem.h>
#include <ECS/Components/ModelComponent.h>
#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/TransformComponent.h>
#include <ECS/Components/LightComponent.h>
#include <ECS/Components/NameComponent.h>
#include <ECS/Components/DoNotSerializeComponent.h>

#include <utils/RenderUtils.h>

namespace alexis
{
	void EditorSystemSerializer::Load(const std::wstring& filename)
	{
		auto& ecsWorld = Core::Get().GetECSWorld();

		fs::path filePath(filename);

		if (!fs::exists(filePath))
		{
			throw std::exception("File not found!");
		}

		using json = nlohmann::json;

		std::ifstream ifs(filename);
		json j = json::parse(ifs);

		auto entities = j["entities"];

		for (auto& entityJson : entities)
		{
			auto entity = ecsWorld.CreateEntity();

			for (auto& [componentName, componentValue] : entityJson["components"].items())
			{
				if (componentName == "CameraComponent")
				{
					float fov = componentValue["fov"];
					float aspectRatio = componentValue["aspectRatio"];
					float nearZ = componentValue["nearZ"];
					float farZ = componentValue["farZ"];
					bool isOrtho = componentValue["isOrtho"] == 1;
					ecsWorld.AddComponent(entity, ecs::CameraComponent{ fov, aspectRatio, nearZ, farZ, isOrtho });
				}
				else if (componentName == "TransformComponent")
				{
					auto posJson = componentValue["position"];
					auto rotJson = componentValue["rotation"];
					auto scaleJson = componentValue["scale"];

					XMVECTOR position = XMVectorSet(posJson["x"], posJson["y"], posJson["z"], 1.f);
					XMVECTOR rotation = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(rotJson["x"]), XMConvertToRadians(rotJson["y"]), XMConvertToRadians(rotJson["z"]));
					float scale = scaleJson;

					ecsWorld.AddComponent(entity, ecs::TransformComponent{ position, rotation, scale });
				}
				else if (componentName == "ModelComponent")
				{
					std::string meshPath = componentValue["mesh"];

					auto* resourceManager = Core::Get().GetResourceManager();
					auto* mesh = resourceManager->GetMesh(ToWStr(meshPath));

					std::string materialPath = componentValue["material"];
					auto* material = resourceManager->GetMaterial(ToWStr(materialPath));

					// TODO: Move semantics for adding components
					ecsWorld.AddComponent(entity, ecs::ModelComponent{ mesh, material });
				}
				else if (componentName == "LightComponent")
				{
					std::string lightTypeStr = componentValue["type"];

					if (lightTypeStr == "Point")
					{
						const auto& colorJson = componentValue["color"];
						XMVECTOR color = XMVectorSet(colorJson["r"], colorJson["g"], colorJson["b"], 1.f);

						ecsWorld.AddComponent(entity, ecs::LightComponent{ ecs::LightComponent::LightType::Point, color });
					}
				}
				else if (componentName == "NameComponent")
				{
					std::string nameStr = componentValue["name"];
					ecsWorld.AddComponent(entity, ecs::NameComponent{ nameStr });
				}
			}
		}
	}

	void EditorSystemSerializer::Save(const std::wstring& filename)
	{
		auto& ecsWorld = Core::Get().GetECSWorld();

		fs::path filePath(filename);

		using json = nlohmann::json;

		std::ofstream ofs(filename);

		json j;


		const auto& editorSystem = ecsWorld.GetSystem<ecs::EditorSystem>();
		const auto& entities = editorSystem->Entities;

		for (auto entity : entities)
		{
			json entityJSON;

			//DoNotSerializeComponent: ignore whole entity
			if (ecsWorld.HasComponent<ecs::DoNotSerializeComponent>(entity))
			{
				continue;
			}

			//TransformComponent
			if (ecsWorld.HasComponent<ecs::TransformComponent>(entity))
			{
				const auto& transformComponent = ecsWorld.GetComponent<ecs::TransformComponent>(entity);
				json transCmp;

				// Position
				{
					XMFLOAT4 position{};
					XMStoreFloat4(&position, transformComponent.Position);
					transCmp["position"]["x"] = position.x;
					transCmp["position"]["y"] = position.y;
					transCmp["position"]["z"] = position.z;
				}

				//Rotation Euler
				{
					XMFLOAT3 rotation = utils::GetPitchYawRollFromQuaternion(transformComponent.Rotation);
					transCmp["rotation"]["x"] = XMConvertToDegrees(rotation.x);
					transCmp["rotation"]["y"] = XMConvertToDegrees(rotation.y);
					transCmp["rotation"]["z"] = XMConvertToDegrees(rotation.z);
				}

				//Scale
				{
					transCmp["scale"] = transformComponent.UniformScale;
				}

				entityJSON["components"]["TransformComponent"] = transCmp;
			}

			//NameComponent
			if (ecsWorld.HasComponent<ecs::NameComponent>(entity))
			{
				const auto& nameComponent = ecsWorld.GetComponent<ecs::NameComponent>(entity);
				json nameCmp;

				nameCmp["name"] = nameComponent.Name;

				entityJSON["components"]["NameComponent"] = nameCmp;
			}

			//ModelComponent
			if (ecsWorld.HasComponent<ecs::ModelComponent>(entity))
			{
				const auto& modelComponent = ecsWorld.GetComponent<ecs::ModelComponent>(entity);
				json modelCmp;

				const auto& meshPath = modelComponent.Mesh->GetPath();
				modelCmp["mesh"] = std::string(meshPath.cbegin(), meshPath.cend());

				const auto& matPath = modelComponent.Material->GetPath();
				modelCmp["material"] = std::string(matPath.cbegin(), matPath.cend());

				entityJSON["components"]["ModelComponent"] = modelCmp;
			}

			//LightComponent
			if (ecsWorld.HasComponent<ecs::LightComponent>(entity))
			{
				const auto& lightComponent = ecsWorld.GetComponent<ecs::LightComponent>(entity);
				json lightCmp;

				lightCmp["type"] = "Point"; // TODO: remove hardcode when other types will be impl
				XMFLOAT4 color{};
				XMStoreFloat4(&color, lightComponent.Color);
				lightCmp["color"]["r"] = color.x;
				lightCmp["color"]["g"] = color.y;
				lightCmp["color"]["b"] = color.z;

				entityJSON["components"]["LightComponent"] = lightCmp;
			}

			//CameraComponent
			if (ecsWorld.HasComponent<ecs::CameraComponent>(entity))
			{
				const auto& cameraComponent = ecsWorld.GetComponent<ecs::CameraComponent>(entity);
				json cameraCmp;

				cameraCmp["fov"] = cameraComponent.Fov;
				cameraCmp["aspectRatio"] = cameraComponent.AspectRatio;
				cameraCmp["nearZ"] = cameraComponent.NearZ;
				cameraCmp["farZ"] = cameraComponent.FarZ;
				cameraCmp["isOrtho"] = cameraComponent.IsOrtho;

				entityJSON["components"]["CameraComponent"] = cameraCmp;
			}

			j["entities"].push_back(entityJSON);
		}

		ofs << j << std::endl;
		ofs.close();
	}

}
