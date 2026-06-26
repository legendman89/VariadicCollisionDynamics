#include "raycast.hpp"
#include <CLibUtilsQTR/DrawDebug.hpp>

bool raycast::castRay(RE::bhkWorld* world, RE::NiPoint3 start, RE::NiPoint3 end, RE::COL_LAYER collisionFilter)
{
	//initialize empty pick datra
	RE::bhkPickData pickData{};
	// get physics world scale
	const float scale = RE::bhkWorld::GetWorldScale();
	// apply scale to start / end points for accurate data
	pickData.rayInput.from = start * scale;
	pickData.rayInput.to = end * scale;
	// enable collision filter to ignore certain collision types
	pickData.rayInput.enableShapeCollectionFilter = true;
	// initialize emtpy filter
	RE::CFilter filter{};
	// first line of filter, our ray will not return a result for any collision type-
	// not belonging to filter  passed in as argument
	filter.SetCollisionLayer(collisionFilter);
	//give ray unique id so it doesent get mixed up with other physic queries
	static const std::uint32_t sSystemGroup =
		RE::bhkCollisionFilter::GetSingleton()->GetNewSystemGroup();
	filter.SetSystemGroup(sSystemGroup);
	pickData.rayInput.filterInfo = filter;
	{
		// must lock the physics world mutex thing or periodic crashes
		RE::BSReadLockGuard lock(world->worldLock);
		world->PickObject(pickData);
	}
	if (!pickData.rayOutput.HasHit())
		return false;
	// get the thing we hit
	auto* collidable = pickData.rayOutput.rootCollidable;
	if (!collidable)
		return false;
	//2nd filter to futher exclude stuff, ignore things like clutter ect (if needed) 
	auto layer = collidable->GetCollisionLayer();
	if (layer != RE::COL_LAYER::kStatic &&
		layer != RE::COL_LAYER::kTerrain &&
		layer != RE::COL_LAYER::kGround)
		return false;

	// debugging
	float totalDistance = start.GetDistance(end);

	// Get mesh of what we hit (I think down to the tri shape that we hit so section of a mesh)
	auto* niObj = RE::TES::GetSingleton()->Pick(pickData);
	if (!niObj || niObj->name.empty())
		return false;

	logger::debug(
		"Mesh above player blocking standing: {} (distance {:.2f})",
		niObj->name.c_str(),
		totalDistance * pickData.rayOutput.hitFraction);

	return true; 
}

 bool raycast::canStandUp(const float a_standingHeight)
{
	 auto player = RE::PlayerCharacter::GetSingleton();
	 if (!player) return false;

	 auto* cell = player->GetParentCell();
	 if (!cell) return false;

	 auto* world = cell->GetbhkWorld();
	 if (!world) return false;
	 
	 // get current position
	 RE::NiPoint3 playerPos = player->GetPosition();

	 RE::NiPoint3 start = playerPos;
	 start.z += 50.0f; // raise start to avoid floor and false positives

	 RE::NiPoint3 end = playerPos;
	 end.z += a_standingHeight; // end at full standing height

	 // see if anything blocking
	 bool canNotStandUp = raycast::castRay(world, start, end, RE::COL_LAYER::kLOS);

	 return !canNotStandUp;
}
