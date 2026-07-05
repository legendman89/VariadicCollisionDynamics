#pragma once

#include "logger.hpp"

namespace Raycast {

	// Casts a ray with a filter for terrain, ground and statics ignores other collision types.
	bool castRay(RE::bhkWorld* world, RE::NiPoint3 start, RE::NiPoint3 end, RE::COL_LAYER collisionFilter);

	bool canStandUp(const float a_standingHeight);

}

