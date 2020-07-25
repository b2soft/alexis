#pragma once

#include <bitset>
#include <cstdint>
#include <queue>
#include <array>
#include <unordered_map>
#include <memory>
#include <set>

namespace alexis
{
	namespace ecs
	{
		// ECS implementation
		// Inspired by https://austinmorlan.com/posts/entity_component_system/#source-code

		// Entity is simple int
		using Entity = std::uint64_t;
		const Entity k_maxEntities = 5000;

		// Int type of component for mask
		using ComponentType = std::uint8_t;
		const ComponentType k_maxComponents = 32;

		// bit mask for components belong to entity
		using ComponentMask = std::bitset<k_maxComponents>;

		class EntityManager
		{
		public:
			EntityManager()
			{
				for (Entity entity = 0; entity < k_maxEntities; ++entity)
				{
					m_availableEntities.push(entity);
				}
			}

			Entity CreateEntity()
			{
				assert(m_livingEntityCount < k_maxEntities && "Total Entity number reached maximum entities per game!");

				Entity id = m_availableEntities.front();
				m_availableEntities.pop();
				++m_livingEntityCount;

				return id;
			}

			void DestroyEntity(Entity entity)
			{
				assert(entity < k_maxEntities && "Entity is out of range!");

				// Invalidate destroyed entity components componentMask
				m_componentMasks[entity].reset();

				m_availableEntities.push(entity);
				--m_livingEntityCount;
			}

			void SetComponentMask(Entity entity, ComponentMask componentMask)
			{
				assert(entity < k_maxEntities && "Entity is out of range!");

				m_componentMasks[entity] = componentMask;
			}

			ComponentMask GetComponentMask(Entity entity)
			{
				assert(entity < k_maxEntities && "Entity is out of range!");

				return m_componentMasks[entity];
			}

		private:
			std::queue<Entity> m_availableEntities;
			std::array<ComponentMask, k_maxEntities> m_componentMasks;

			std::uint32_t m_livingEntityCount{ 0 };
		};

		// The one instance of virtual inheritance in the entire implementation.
		// An interface is needed so that the ComponentManager (seen later)
		// can tell a generic ComponentArray that an entity has been destroyed
		// and that it needs to update its array mappings.
		class IComponentArray
		{
		public:
			virtual ~IComponentArray() = default;
			virtual void EntityDestroyed(Entity entity) = 0;
		};

		template<class T>
		class ComponentArray : public IComponentArray
		{
		public:
			void InsertData(Entity entity, T& component)
			{
				assert(m_entityToIndexMap.find(entity) == m_entityToIndexMap.end() && "Trying add component to Entity that already has it!");

				std::size_t newIndex = m_size;
				m_entityToIndexMap[entity] = newIndex;
				m_indexToEntityMap[newIndex] = entity;
				m_componentArray[newIndex] = std::move(component);
				++m_size;
			}

			void RemoveData(Entity entity)
			{
				// Copy last elem to deleted elem
				std::size_t indexOfRemovedEntity = m_entityToIndexMap[entity];
				std::size_t indexOfLastElement = m_size - 1;

				std::swap(m_componentArray[indexOfRemovedEntity], m_componentArray[indexOfLastElement]);

				// Update map to point to moved spot
				Entity entityOfLastElement = m_indexToEntityMap[indexOfLastElement];
				m_entityToIndexMap[entityOfLastElement] = indexOfRemovedEntity;
				m_indexToEntityMap[indexOfRemovedEntity] = entityOfLastElement;

				m_entityToIndexMap.erase(entity);
				m_indexToEntityMap.erase(indexOfLastElement);
				--m_size;
			}

			T& GetData(Entity entity)
			{
				assert(m_entityToIndexMap.find(entity) != m_entityToIndexMap.end() && "Retrieving non-existent component.");

				return m_componentArray[m_entityToIndexMap[entity]];
			}

			bool HasData(Entity entity)
			{
				return m_entityToIndexMap.contains(entity);
			}

			void EntityDestroyed(Entity entity) override
			{
				// Remove component when entity is destroyed
				if (m_entityToIndexMap.find(entity) != m_entityToIndexMap.end())
				{
					RemoveData(entity);
				}
			}

		private:
			// Packed array of component of type T
			std::array<T, k_maxEntities> m_componentArray;

			// Map from entity ID to array index
			std::unordered_map<Entity, std::size_t> m_entityToIndexMap;

			// Map from array index to entity ID
			std::unordered_map<Entity, std::size_t> m_indexToEntityMap;

			// Total number of valid entries in array
			std::size_t m_size{ 0 };
		};

		// Component Manager
		class ComponentManager
		{
		public:
			template<class T>
			void RegisterComponent()
			{
				const char* typeName = typeid(T).name();

				assert(m_componentTypes.find(typeName) == m_componentTypes.end() && "Component has been already registered!");

				m_componentTypes.insert({ typeName, m_nextComponentType });

				m_componentArrays.insert({ typeName, std::make_shared<ComponentArray<T>>() });

				++m_nextComponentType;
			}

			template<class T>
			ComponentType GetComponentType()
			{
				const char* typeName = typeid(T).name();

				assert(m_componentTypes.find(typeName) != m_componentTypes.end() && "Component does not exist!");

				return m_componentTypes[typeName];
			}

			template<class T>
			void AddComponent(Entity entity, T& component)
			{
				GetComponentArray<T>()->InsertData(entity, component);
			}

			template<class T>
			void RemoveComponent(Entity entity, T component)
			{
				GetComponentArray<T>()->RemoveData(entity, component);
			}

			template<class T>
			T& GetComponent(Entity entity)
			{
				return GetComponentArray<T>()->GetData(entity);
			}

			template<class T>
			bool HasComponent(Entity entity)
			{
				return GetComponentArray<T>()->HasData(entity);
			}

			void EntityDestroyed(Entity entity)
			{
				// Notify each component array that entity has been destroyed
				for (const auto& pair : m_componentArrays)
				{
					const auto& component = pair.second;

					component->EntityDestroyed(entity);
				}
			}

		private:
			// Map from type string pointer to a component type
			std::unordered_map<const char*, ComponentType> m_componentTypes;

			// Map from type string pointer to a component array
			std::unordered_map<const char*, std::shared_ptr<IComponentArray>> m_componentArrays;

			// Component type to be assigned for next registered component
			ComponentType m_nextComponentType{ 0 };

			template<class T>
			std::shared_ptr<ComponentArray<T>> GetComponentArray()
			{
				const char* typeName = typeid(T).name();

				assert(m_componentTypes.find(typeName) != m_componentTypes.end() && "Component is not registered!");

				return std::static_pointer_cast<ComponentArray<T>>(m_componentArrays[typeName]);
			}
		};

		// Systems

		struct System
		{
			std::set<Entity> Entities;
		};

		class SystemManager
		{
		public:
			template<class T>
			std::shared_ptr<T> RegisterSystem()
			{
				const char* typeName = typeid(T).name();

				assert(m_systems.find(typeName) == m_systems.end() && "System has been already registered!");

				auto system = std::make_shared<T>();
				m_systems.insert({ typeName, system });
				return system;
			}

			template<class T>
			std::shared_ptr<T> GetSystem()
			{
				const char* typeName = typeid(T).name();

				auto it = m_systems.find(typeName);
				assert(it != m_systems.end() && "System not found!");

				return std::static_pointer_cast<T>(it->second);
			}

			template<class T>
			void SetComponentMask(ComponentMask componentMask)
			{
				const char* typeName = typeid(T).name();

				auto it = m_systems.find(typeName);
				assert(it != m_systems.end() && "System not found!");

				m_componentMasks.insert({ typeName, componentMask });
			}

			void EntityDestroyed(Entity entity)
			{
				for (const auto& pair : m_systems)
				{
					const auto& system = pair.second;

					system->Entities.erase(entity);
				}
			}

			void EntityComponentMaskChanged(Entity entity, ComponentMask componentMask)
			{
				// Notify all systems that entity componentMask was changed
				for (const auto& pair : m_systems)
				{
					const auto& type = pair.first;
					const auto& system = pair.second;
					const auto& systemComponentMask = m_componentMasks[type];

					if ((componentMask & systemComponentMask) == systemComponentMask)
					{
						system->Entities.insert(entity);
					}
					else
					{
						system->Entities.erase(entity);
					}
				}
			}

		private:
			// Map System type -> component mask
			std::unordered_map<const char*, ComponentMask> m_componentMasks;

			// Map System type to System pointer
			std::unordered_map<const char*, std::shared_ptr<System>> m_systems;
		};;

		// ECS World
		class World
		{
		public:
			void Init()
			{
				m_componentManager = std::make_unique<ComponentManager>();
				m_entityManager = std::make_unique<EntityManager>();
				m_systemManager = std::make_unique<SystemManager>();
			}

			Entity CreateEntity()
			{
				return m_entityManager->CreateEntity();
			}

			void DestroyEntity(Entity entity)
			{
				m_entityManager->DestroyEntity(entity);
				m_componentManager->EntityDestroyed(entity);
				m_systemManager->EntityDestroyed(entity);
			}

			// Component related
			template<class T>
			void RegisterComponent()
			{
				m_componentManager->RegisterComponent<T>();
			}

			template<class T>
			void AddComponent(Entity entity, T component)
			{
				m_componentManager->AddComponent<T>(entity, component);

				auto componentMask = m_entityManager->GetComponentMask(entity);
				componentMask.set(m_componentManager->GetComponentType<T>(), true);
				m_entityManager->SetComponentMask(entity, componentMask);

				m_systemManager->EntityComponentMaskChanged(entity, componentMask);
			}

			template<class T>
			void RemoveComponent(Entity entity, T component)
			{
				m_componentManager->RemoveComponent<T>(entity, component);

				auto componentMask = m_entityManager->GetComponentMask(entity);
				componentMask.set(m_componentManager->GetComponentType<T>(), false);
				m_entityManager->SetComponentMask(entity, componentMask);

				m_systemManager->EntityComponentMaskChanged(entity, componentMask);
			}

			template<class T>
			T& GetComponent(Entity entity)
			{
				return m_componentManager->GetComponent<T>(entity);
			}

			template<class T>
			bool HasComponent(Entity entity)
			{
				return m_componentManager->HasComponent<T>(entity);
			}

			template<class T>
			ComponentType GetComponentType()
			{
				return m_componentManager->GetComponentType<T>();
			}

			// System related
			template<class T>
			std::shared_ptr<T> RegisterSystem()
			{
				return m_systemManager->RegisterSystem<T>();
			}

			template<class T>
			void SetSystemComponentMask(ComponentMask componentMask)
			{
				m_systemManager->SetComponentMask<T>(componentMask);
			}

			template<class T>
			std::shared_ptr<T> GetSystem()
			{
				return m_systemManager->GetSystem<T>();
			}

		private:
			std::unique_ptr<ComponentManager> m_componentManager;
			std::unique_ptr<EntityManager> m_entityManager;
			std::unique_ptr<SystemManager> m_systemManager;
		};
	}
}