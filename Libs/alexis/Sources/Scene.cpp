#include <Precompiled.h>

#include "Scene.h"

#include <Core/Core.h>
#include <json.hpp>
#include <fstream>

#include <ECS/ECS.h>
#include <ECS/CameraComponent.h>
#include <ECS/TransformComponent.h>

namespace alexis
{

	void Scene::LoadFromJson(const std::wstring& filename)
	{
		auto ecsWorld = Core::Get().GetECS();

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
			auto entity = ecsWorld->CreateEntity();

			for (auto& component : entityJson["components"])
			{
				auto componentKV = component.items().begin();
				auto componentName = componentKV.key();

				if (componentName == "CameraComponent")
				{
					float fov = componentKV.value()["fov"];
					float aspectRatio = componentKV.value()["aspectRatio"];
					float nearZ = componentKV.value()["nearZ"];
					float farZ = componentKV.value()["farZ"];
					ecsWorld->AddComponent(entity, ecs::CameraComponent{ fov, aspectRatio, nearZ, farZ });
				}
				else if (componentName == "TransformComponent")
				{
					auto posJson = componentKV.value()["position"];
					auto rotJson = componentKV.value()["rotation"];

					XMVECTOR position = XMVectorSet(posJson["x"], posJson["y"], posJson["z"], 1.f);
					XMVECTOR rotation = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(rotJson["x"]), XMConvertToRadians(rotJson["y"]), XMConvertToRadians(rotJson["z"]));

					ecsWorld->AddComponent(entity, ecs::TransformComponent{position, rotation});
				}
				else if (componentName == "ModelComponent")
				{
					// TODO
				}
			}
		}
	}

}