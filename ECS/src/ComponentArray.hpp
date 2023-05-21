#pragma once

#include "Common.hpp"

#include <unordered_map>
#include <ranges>

namespace ECS
{
	class IComponentArray
	{
	public:
		virtual ~IComponentArray() = default;
		virtual void OnEntityDestroyed(const Entity entity) = 0;
	};

	// This class is a data structure that is in itself a *packed* array, meaning there are no *holes* in the array.
	// When a new entry is added it will always be added at the end
	// When an entry is removed the last entry in the array will be moved to the place the first entry was removed from
	// This way it is ensured the data will always be packed together making it cache friendly
	template<typename T>
	class ComponentArray final : public IComponentArray
	{
	public:
		using Index = size_t;
		using ValueType = T;

		// Iterator to be used to go through every instance of component T
		class Iterator
		{
		public:
			using ValueType = T;
			using Pointer = ValueType*;
			using Reference = ValueType&;

			Iterator(T* componentArray, Index index)
				: m_ComponentArray(componentArray), m_Index(index)
			{}

			Iterator& operator++()
			{
				return --m_Index, *this;
			}

			Iterator operator++(int)
			{
				Iterator original = *this;
				return ++(*this), original;
			}

			Iterator& operator--()
			{
				return ++m_Index, *this;
			}

			Iterator operator--(int)
			{
				Iterator original = *this;
				return operator--(), original;
			}

			bool operator==(const Iterator& other) const
			{
				return m_Index == other.m_Index;
			}

			bool operator!=(const Iterator& other) const
			{
				return !(*this == other);
			}

			[[nodiscard]] Pointer operator->() const
			{
				const auto pos = m_Index - 1;
				return &m_ComponentArray[pos];
			}

			[[nodiscard]] Reference operator*() const
			{
				return *operator->();
			}

		private:
			T* m_ComponentArray;
			Index m_Index;
		};

		// Iterator to be used to go through all the Entities with component T (in this array instance)
		class EntityIterator
		{
			using Map = std::unordered_map<Entity, Index>;
			using ValueType = const Entity;
			using Pointer = ValueType*;
			using Reference = ValueType&;
		
		public:
			explicit EntityIterator(const Map& entityToIndexMap, const bool isEnd = false)
				: m_EntityToIndexMap(entityToIndexMap)
			{
				m_CurrentIterator = isEnd ? m_EntityToIndexMap.end() : m_EntityToIndexMap.begin();
			}
		
			EntityIterator& operator++()
			{
				++m_CurrentIterator;
				return *this;
			}
		
			EntityIterator& operator++(int)
			{
				EntityIterator original = *this;
				return ++(*this), original;
			}
		
			EntityIterator& operator--()
			{
				--m_CurrentIterator;
				return *this;
			}
		
			EntityIterator& operator--(int)
			{
				EntityIterator original = *this;
				return operator--(), original;
			}
		
			bool operator==(const EntityIterator& other) const
			{
				return m_CurrentIterator == other.m_CurrentIterator;
			}
		
			bool operator!=(const EntityIterator& other) const
			{
				return !(*this == other);
			}

			Pointer operator->() const
			{
				return &m_CurrentIterator->first;
			}

			Reference operator*() const
			{
				return *operator->();
			}
		
		private:
			const Map& m_EntityToIndexMap;
			Map::const_iterator m_CurrentIterator;
		};

	public:
		template<typename ... Args>
		T& InsertData(const Entity entity, Args&& ... args)
		{
			// Set the index and the entity in the maps
			Index newIndex = m_Size;
			m_EntityToIndexMap[entity] = newIndex;
			m_IndexToEntityMap[newIndex] = entity;

			// Grab a ref to the block of memory where this component will be stored and initialize it
			T& newComponent = m_ComponentArray[newIndex];
			newComponent = T(std::forward<Args>(args)...);

			// Increase the size for the next index and return the ref to the memory containing the new component
			m_Size++;
			return newComponent;
		}

		void RemoveData(const Entity entity)
		{
			// Copy the element at the end into the deleted element's place
			Index indexOfRemovedEntity = m_EntityToIndexMap[entity];
			Index indexOfLastElement = m_Size - 1;
			m_ComponentArray[indexOfRemovedEntity] = m_ComponentArray[indexOfLastElement];

			// Update the map to point to the moved spot
			Entity entityOfLastElement = m_IndexToEntityMap[indexOfLastElement];
			m_EntityToIndexMap[entityOfLastElement] = indexOfRemovedEntity;
			m_IndexToEntityMap[indexOfRemovedEntity] = entityOfLastElement;

			m_EntityToIndexMap.erase(entity);
			m_IndexToEntityMap.erase(indexOfLastElement);

			m_Size--;
		}

		T& GetData(const Entity entity)
		{
			return m_ComponentArray[m_EntityToIndexMap[entity]];
		}

		bool HasData(const Entity entity) const
		{
			return m_EntityToIndexMap.contains(entity);
		}

		void OnEntityDestroyed(const Entity entity) override
		{
			if (m_EntityToIndexMap.contains(entity))
				RemoveData(entity);
		}

		// Default iterator, (to be used with range-based for loop) will go through the components
		[[nodiscard]] Iterator begin() { return Iterator(m_ComponentArray, m_Size); }
		[[nodiscard]] Iterator end() { return Iterator(m_ComponentArray, {}); }

		// Extra iterator, will go through entities ids
		[[nodiscard]] EntityIterator BeginEntityIt() { return EntityIterator(m_EntityToIndexMap, false); }
		[[nodiscard]] EntityIterator EndEntityIt() { return EntityIterator(m_EntityToIndexMap, true); }

	private:
		T m_ComponentArray[MAX_ENTITIES]{};
		std::unordered_map<Entity, Index> m_EntityToIndexMap;
		std::unordered_map<Index, Entity> m_IndexToEntityMap;
		Index m_Size = 0;
	};
}
