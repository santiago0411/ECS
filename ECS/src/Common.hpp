#pragma once

#include <iostream>

namespace ECS
{
	constexpr uint32_t MAX_ENTITIES = 50000;
	constexpr uint16_t MAX_COMPONENTS = 32;

	using Entity = uint32_t;
	using ComponentTypeId = uint32_t;
}
