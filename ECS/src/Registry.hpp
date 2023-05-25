#pragma once

#include "Common.hpp"
#include "ComponentArray.hpp"
#include "View.hpp"

#include <queue>
#include <cassert>
#include <ranges>
#include <memory>
#include <unordered_map>

namespace ECS
{
	// This class manages all the entities lifetimes and provides functionality to
	// add, get, remove components to/from entities as well as to pack them in a view
	// to iterate over a set of entities that have the same components
	class Registry
	{
	public:
		Registry()
		{
			for (Entity id = 0; id < MAX_ENTITIES; id++)
				m_AvailableEntities.push(id);
		}

		[[nodiscard]] Entity CreateEntity()
		{
			assert(m_LivingEntityCount < MAX_ENTITIES && "Too many entities in existance.");

			Entity id = m_AvailableEntities.front();
			m_AvailableEntities.pop();
			m_LivingEntityCount++;

			return id;
		}

		void DestroyEntity(const Entity entity)
		{
			assert(entity < MAX_ENTITIES && "Entity out of range.");

			OnEntityDestroyed(entity);
			m_AvailableEntities.push(entity);
			m_LivingEntityCount--;
		}

		template<typename T, typename ... Args>
		T& AddComponent(Entity entity, Args&& ... args)
		{
			return GetComponentArray<T>()->InsertData(entity, std::forward<Args>(args)...);
		}

		template<typename T>
		[[nodiscard]] T& GetComponent(Entity entity)
		{
			return GetComponentArray<T>()->GetData(entity);
		}

		template<typename T>
		bool HasComponent(Entity entity)
		{
			return GetComponentArray<T>()->HasData(entity);
		}

		template<typename T>
		void RemoveComponent(Entity entity)
		{
			GetComponentArray<T>()->RemoveData(entity);
		}

		template<typename ... Components>
		[[nodiscard]] View<Components...> View()
		{
			static_assert(sizeof...(Components) > 0, "Cannot make a view with no components");
			return { GetComponentArray<Components>() ... };
		}

		template<typename Comp, typename Func>
		void OnConstruct(Func func)
		{
			std::shared_ptr<ComponentArray<Comp>> array = GetComponentArray<Comp>();
			static_assert(std::is_invocable_v<Func, Entity, Comp&>, "OnConstruct func doesn't take the correct type of args.");
			array->RegisterOnConstruct(func);
		}

		template<typename Comp, typename Func>
		void OnDestroy(Func func)
		{
			std::shared_ptr<ComponentArray<Comp>> array = GetComponentArray<Comp>();
			static_assert(std::is_invocable_v<Func, Entity, Comp&>, "OnDestroy func doesn't take the correct type of args.");
			array->RegisterOnDestroy(func);
		}

	private:
		inline static ComponentTypeId GetUniqueComponentId()
		{
			static ComponentTypeId lastId = 0;
			return lastId++;
		}

		template<typename T>
		inline static ComponentTypeId GetComponentTypeId() noexcept
		{
			// Because this is a templated function a different specification will be created
			// for each type that it is called with. Because TYPE_ID is static it's value
			// will only ever be assigned once in the whole program's lifetime.
			// Thus creating a unique id for each component type every time.
			static const ComponentTypeId TYPE_ID = GetUniqueComponentId();
			return TYPE_ID;
		}

		template<typename T>
		std::shared_ptr<ComponentArray<T>> GetComponentArray()
		{
			const ComponentTypeId typeId = GetComponentTypeId<T>();
			std::shared_ptr<IComponentArray>& arr = m_ComponentArrays[typeId];
			if (!arr)
				arr = std::make_shared<ComponentArray<T>>();

			return std::static_pointer_cast<ComponentArray<T>>(arr);
		}

		void OnEntityDestroyed(const Entity entity)
		{
			// Notify each component array that an entity has been destroyed
			for (const auto& componentArray : m_ComponentArrays | std::views::values)
				componentArray->OnEntityDestroyed(entity);

			// Maybe should remove the array from the map to free the memory here
			// Having a static array with a set size isn't very memory efficient anyways
		}

	private:
		std::queue<Entity> m_AvailableEntities;
		uint32_t m_LivingEntityCount = 0;

		std::unordered_map<ComponentTypeId, std::shared_ptr<IComponentArray>> m_ComponentArrays;
	};
}
