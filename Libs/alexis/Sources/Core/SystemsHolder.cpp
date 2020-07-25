#include "Precompiled.h"

#include "SystemsHolder.h"

#include "Core/Core.h"

#include <ECS/ECS.h>
#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/ModelComponent.h>
#include <ECS/Components/TransformComponent.h>
#include <ECS/Components/LightComponent.h>
#include <ECS/Components/NameComponent.h>
#include <ECS/Components/DoNotSerializeComponent.h>

#include <ECS/Systems/ModelSystem.h>
#include <ECS/Systems/ShadowSystem.h>
#include <ECS/Systems/CameraSystem.h>
#include <ECS/Systems/LightingSystem.h>
#include <ECS/Systems/Hdr2SdrSystem.h>
#include <ECS/Systems/ImguiSystem.h>
#include <ECS/Systems/EnvironmentSystem.h>
#include <ECS/Systems/EditorSystem.h>

namespace alexis
{
	void SystemsHolder::Init()
	{
		Register();
		InitInternal();
	}

	void SystemsHolder::Register()
	{
		auto& ecsWorld = Core::Get().GetECSWorld();

		ecsWorld.RegisterComponent<ecs::ModelComponent>();
		ecsWorld.RegisterComponent<ecs::TransformComponent>();
		ecsWorld.RegisterComponent<ecs::CameraComponent>();
		ecsWorld.RegisterComponent<ecs::LightComponent>();
		ecsWorld.RegisterComponent<ecs::NameComponent>();
		ecsWorld.RegisterComponent<ecs::DoNotSerializeComponent>();

		// Model System
		m_modelSystem = ecsWorld.RegisterSystem<ecs::ModelSystem>();

		ecs::ComponentMask modelSystemMask;
		modelSystemMask.set(ecsWorld.GetComponentType<ecs::ModelComponent>());
		modelSystemMask.set(ecsWorld.GetComponentType<ecs::TransformComponent>());
		ecsWorld.SetSystemComponentMask<ecs::ModelSystem>(modelSystemMask);

		// Shadow System
		m_shadowSystem = ecsWorld.RegisterSystem<ecs::ShadowSystem>();

		ecs::ComponentMask shadowSystemMask;
		shadowSystemMask.set(ecsWorld.GetComponentType<ecs::ModelComponent>());
		shadowSystemMask.set(ecsWorld.GetComponentType<ecs::TransformComponent>());
		ecsWorld.SetSystemComponentMask<ecs::ShadowSystem>(shadowSystemMask);

		// Camera System
		m_cameraSystem = ecsWorld.RegisterSystem<ecs::CameraSystem>();

		ecs::ComponentMask cameraSystemMask;
		cameraSystemMask.set(ecsWorld.GetComponentType<ecs::CameraComponent>());
		cameraSystemMask.set(ecsWorld.GetComponentType<ecs::TransformComponent>());
		ecsWorld.SetSystemComponentMask<ecs::CameraSystem>(cameraSystemMask);

		m_environmentSystem = ecsWorld.RegisterSystem<ecs::EnvironmentSystem>();

		// Lighting System
		m_lightingSystem = ecsWorld.RegisterSystem<ecs::LightingSystem>();

		ecs::ComponentMask lightingSystemMask;
		lightingSystemMask.set(ecsWorld.GetComponentType<ecs::LightComponent>());
		lightingSystemMask.set(ecsWorld.GetComponentType<ecs::TransformComponent>());
		ecsWorld.SetSystemComponentMask<ecs::LightingSystem>(lightingSystemMask);

		m_hdr2SdrSystem = ecsWorld.RegisterSystem<ecs::Hdr2SdrSystem>();

		m_imguiSystem = ecsWorld.RegisterSystem<ecs::ImguiSystem>();


		m_editorSystem = ecsWorld.RegisterSystem<ecs::EditorSystem>();

		ecs::ComponentMask editorSystemMask;
		ecsWorld.SetSystemComponentMask<ecs::EditorSystem>(editorSystemMask);
	}

	void SystemsHolder::InitInternal()
	{
		//TODO: Temp solution: moved env system before Lighting. RT creating needs refactoring (create without order or cache names instead direct init)
		m_environmentSystem->Init();

		m_lightingSystem->Init();
		m_shadowSystem->Init();
		m_hdr2SdrSystem->Init();
		m_imguiSystem->Init();
	}

}