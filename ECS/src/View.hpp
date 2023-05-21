#pragma once

#include "ComponentArray.hpp"

#include <tuple>
#include <memory>
#include <cassert>

namespace ECS
{
	// These 3 structs are used to determine if a given function type 'Func' is
	// invocable with a tuple of arguments 'Tuple<Args...>'

	// Default case when no specialization matches
	template<typename, typename>
	struct IsApplicable : std::false_type {};

	// Case when Func is invocable with arguments Tuple<Args...>
	template<typename Func, template<typename ...> class Tuple, typename ... Args>
	struct IsApplicable<Func, Tuple<Args...>> : std::is_invocable<Func, Args...> {};

	// Case when Func is invocable with arguments const Tuple<Args...>
	template<typename Func, template<typename ...> class Tuple, typename ... Args>
	struct IsApplicable<Func, const Tuple<Args...>> : std::is_invocable<Func, Args...> {};

	// Short-hand to check the value of the result
	template<typename Func, typename Args>
	inline constexpr bool IsApplicableV = IsApplicable<Func, Args>::value;

	// Lightweight class that provides functionality to group entities together based on
	// a provided set of components and provides different ways to iterate over them
	template<typename ... Component>
	class View
	{
		template<typename T>
		using Storage = std::shared_ptr<ComponentArray<T>>;

	public:
		// Returns a tuple with the components of the specified template types for a given entity
		// Returns all components registered in the view if no template type is specified
		template<typename ... Comp>
		[[nodiscard]] decltype(auto) Get(const Entity entity)
		{
			assert(Contains(entity) && "View does not contain entity");

			// Case 0 types were provided, so use class template 'Component' and return all types
			if constexpr (sizeof...(Comp) == 0)
				return std::tuple_cat(GetAsTuple(*std::get<Storage<Component>>(m_Arrays), entity)...);

			// Case a single type was provided, only retrieve data for that array type
			else if constexpr (sizeof...(Comp) == 1)
				return (std::get<Storage<Comp>>(m_Arrays)->GetData(entity), ...);

			// One or more types were provided, same as the first case but use the function template instead
			else
				return std::tuple_cat(GetAsTuple(*std::get<Storage<Comp>>(m_Arrays), entity)...);
		}

		// Returns whether or not an entity exists in this view
		[[nodiscard]] bool Contains(const Entity entity) const
		{
			return (std::get<Storage<Component>>(m_Arrays)->HasData(entity) && ...);
		}

		// This iterator will go through every entity that has all the components specified in the view template
		[[nodiscard]] decltype(auto) begin() const { return std::get<0>(m_Arrays)->BeginEntityIt(); }
		[[nodiscard]] decltype(auto) end() const { return std::get<0>(m_Arrays)->EndEntityIt(); }

		// This function accepts a lambda that takes all the component types specified in the view as arguments
		// it may also have 'Entity' as its first argument type if desired. The argument types must be in the
		// same order as they were used to create the view.
		// It will be called once per entity that was grouped in this view.
		template<typename Func>
		void Each(Func func) const
		{
			// Get the array of whatever the first type is and it's component iterator
			const auto arr = std::get<0>(m_Arrays);
			auto compIt = arr->begin();

			// Iterate through all the entities in the array
			for (auto entityIt = arr->BeginEntityIt(); entityIt != arr->EndEntityIt(); ++entityIt)
			{
				const Entity entity = *entityIt;

				// Pack all the component types into a tuple and call the lambda either with or without Entity as its first arg
				if constexpr (IsApplicableV<Func, decltype(std::tuple_cat(std::tuple<Entity>{}, std::declval<View>().Get({})))>)
					std::apply(func, std::tuple_cat(std::make_tuple(entity), DispatchGet<Component>(compIt, entity)...));
				else
					std::apply(func, std::tuple_cat(DispatchGet<Component>(compIt, entity)...));

				++compIt;
			}
		}

	private:
		View(Storage<Component> ... componentArrays)
			: m_Arrays(componentArrays...) {}

		template<typename Type>
		auto GetAsTuple(Type& container, const Entity entity) const
		{
			// Return an empty tuple if the data type is void or create a tuple with the component data
			if constexpr (std::is_void_v<decltype(container.GetData({}))>)
				return std::make_tuple();
			else
				return std::forward_as_tuple(container.GetData(entity));
		}

		template<typename Comp, typename It>
		auto DispatchGet(It& it, const Entity entity) const
		{
			// If the ComponentArray iterator being used matches the type of the component that
			// is being retrieved just dereference the it and return it. (This avoids unnecessary indirection
			// since calling GetAsTuple with go through the maps in ComponentArray to find the correct index
			// and the iterator already has the index needed)
			if constexpr (std::is_same_v<typename It::ValueType, typename ComponentArray<Comp>::ValueType>)
				return std::forward_as_tuple(*it);
			// Otherwise call GetAsTuple to get the data through the corresponding array
			else
				return GetAsTuple(*std::get<Storage<Comp>>(m_Arrays), entity);
		}

	private:
		friend class Registry;
		const std::tuple<Storage<Component> ...> m_Arrays;
	};
}