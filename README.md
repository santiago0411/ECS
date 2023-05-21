# ECS (Entity Component System)

A simple implementation of an entity component system for a game engine. API based on [entt](https://github.com/skypjack/entt).
Provides classes to create/destroy entities and to add any type of components to them. As well as a 'View' to iterate over all existing entities have all the provided types. The component data of a specific type is packed together in an array so that iterating over it will yield more cpu cache hits.
Using C++ powerful template system the View class provides a way to mix and match any type of components and iterate over them.

## Usage Example

```cpp
#include <iostream>
#include <format>

#include "Registry.hpp"

struct UUIDComponent
{
	uint64_t Id;
};

struct TagComponent
{
	std::string Name;
};

struct PointComponent
{
	int X, Y;

	std::string ToString() const
	{
		return std::format("({}, {})", X, Y);
	}
};

int main()
{
	using namespace ECS;

	// Create a new registry
	Registry registry;

	// Create entities
	Entity entity1 = registry.CreateEntity();
	Entity entity2 = registry.CreateEntity();

	// Add components to them with different values
	registry.AddComponent<UUIDComponent>(entity1, 5);
	registry.AddComponent<TagComponent>(entity1, "Pepe");
	registry.AddComponent<PointComponent>(entity1, 1, 2);

	registry.AddComponent<UUIDComponent>(entity2, 10);
	registry.AddComponent<TagComponent>(entity2, "Tom");
	registry.AddComponent<PointComponent>(entity2, 3, 4);

	// Create a view that will match every entity that has components of type
	// UUIDComponent, TagComponent, PointComponent, in this case both entities will match
	// because both have all three types
	View view = registry.View<UUIDComponent, TagComponent, PointComponent>();

	std::cout << "View.Each() with Entity:\n\n";
	// First way to iterate over all the entities in the view using View.Each()
	view.Each([](Entity entity, UUIDComponent& uuid, TagComponent& tc, PointComponent& pc)
	{
		std::cout << "- Entity: " << entity << 
			"\n\tId: " << uuid.Id <<
			"\n\tName: " << tc.Name << 
			"\n\tPoint: " << pc.ToString() << "\n\n";
	});

	std::cout << "\nView.Each() with no Entity:\n\n";
	// Same as above but it can also not have entity as the lambda's first argument
	view.Each([](UUIDComponent& uuid, TagComponent& tc, PointComponent& pc)
	{
		std::cout << "Id: " << uuid.Id <<
			"\nName: " << tc.Name <<
			"\nPoint: " << pc.ToString() << "\n\n";
	});

	std::cout << "\nRanged-based loop on the view:\n\n";
	// Iterate over the view using a range-based loop
	for (const Entity entity : view)
	{
		// Get the tuple with all the components using view.Get(entity)
		// Using C++17 structured bindings there is no need to have
		// std::tuple<UUIDComponent, TagComponent, PointComponent> as the return type
		// The get function also accepts template arguments if you only want some of the components
		// i.e. const auto& [tagComponent] = view.Get<TagComponent>(entity); 
		const auto& [uuid, tc, pc] = view.Get(entity);
		std::cout << "- Entity: " << entity <<
			"\n\tId: " << uuid.Id <<
			"\n\tName: " << tc.Name <<
			"\n\tPoint: " << pc.ToString() << "\n\n";
	}

	// Finally destroy the entities from the registry
	registry.DestroyEntity(entity1);
	registry.DestroyEntity(entity2);

	return 0;
}
```