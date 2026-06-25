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
	float hitDistance = totalDistance * pickData.rayOutput.hitFraction;

	// get mesh of what we hit (I think down to the tri shape that we hit so section of a mesh
	auto* niObj = RE::TES::GetSingleton()->Pick(pickData);
	if (!niObj || niObj->name.empty())
		return false;

	logger::info(
		"mesh above player blocking standing: {} (distance {:.2f})",
		niObj->name.c_str(),
		hitDistance);

	return true; 
}

 bool raycast::canStandUp(float a_standingHeight)
{
	 auto player = RE::PlayerCharacter::GetSingleton();
	 if (!player) return false;

	 auto* cell = player->GetParentCell();
	 if (!cell) return false;

	 auto* world = cell->GetbhkWorld();
	 if (!world) return false;
	 
	 // get current position
	 RE::NiPoint3 playerPos = player->GetPosition();

	 //apply tiny offset to avoid hitting bumpy terrain
	 playerPos.z += 50; 

	 // subtract z offset applied to player pos above
	 a_standingHeight -= 50.0f; 

	 //start the ray from players current position (may need to adjust this if Z is not ground level) 
	 RE::NiPoint3 start = playerPos;
	 RE::NiPoint3 end = playerPos;

	 // set max distance of ray to the players height, subtract z offset from earlier
	 end.z += a_standingHeight;

	 // see if anything blocking
	 bool canNotStandUp = raycast::castRay(world, start, end, RE::COL_LAYER::kLOS);
	 
	 //DEBUG lines to view the ray 
	 
	/* if (canNotStandUp) {
		
		 RE::NiColorA rayColor = canNotStandUp ? RE::NiColorA{1.0f, 0.0f, 0.0f, 1.0f} : RE::NiColorA{0.0f, 1.0f, 0.0f, 1.0f};
		 auto drawAPI = DebugAPI_IMPL::DebugAPI::GetSingleton();
		 if (!drawAPI) return true;

		 drawAPI->DrawLineForMS(start, end, 100, rayColor, 2.0f);
		 drawAPI->Update();
		 logger::info("drawing debug line");
	 }*/

	 return !canNotStandUp;
}
