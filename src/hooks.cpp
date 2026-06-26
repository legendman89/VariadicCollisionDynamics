 
#include "draw.hpp"
#include "hooks.hpp"
#include "helper.hpp"
#include "logger.hpp"
#include "manager.hpp"
#include "dynamics.hpp"
#include "settings.hpp"
#include "raycast.hpp"
#include "posefixes.hpp"
#include <CLibUtilsQTR/DrawDebug.hpp>

using namespace Hook; 
using namespace DebugAPI_IMPL;

void PlayerUpdate::thunk(RE::PlayerCharacter* player, float delta) {

	func(player, delta);

	if (!player) {
		logger::warn("Player is null, cannot update.");
		return;
	}

	static bool initialized = false;

	if (!initialized) {
		initialized = true;

		const auto playerIsSneaking = player->IsSneaking();

		logger::info("player is sneaking on startup = {}", playerIsSneaking);

		//set playerIsSneaking when first loaded
		if (playerIsSneaking && Settings::GetSettings().fixPlayerSneaking) {
			auto& manager = VCD::Manager::GetSingleton();
			manager.FixSneakingPose(player, PoseFixes::PlayerPose(player), false);
		}
	}

	Dynamics::Update(player);

	Dynamics::UpdateNPCs(player);

	const auto& settings = Settings::GetSettings();
	if (settings.drawCollision || settings.drawNearbyActors) {
		DebugAPI::GetSingleton()->LinesToDraw.clear();
	}

	if (settings.drawCollision) {
		Draw::DrawPlayerBumper();
	}

	if (settings.drawNearbyActors) {
		Draw::DrawNearbyActorBumpers();
	}

	if (settings.drawCollision || settings.drawNearbyActors) {
		DebugAPI::GetSingleton()->Update();
	}
}

void PlayerUpdate::Install()
{
	func = REL::Relocation<std::uintptr_t>(RE::PlayerCharacter::VTABLE[0]).write_vfunc(0xAD, thunk);
	logger::info("Player update hook installed.");
}

// isUp() called only once per sneak key press
void SneakHandlerProcessButton::thunk(
	RE::SneakHandler* a_this,
	RE::ButtonEvent* a_event,
	RE::PlayerControlsData* a_data)
{

	if (!a_this || !a_event || !a_data)
		return func(a_this, a_event, a_data);

	if (!Settings::GetSettings().fixPlayerSneaking) {
		return func(a_this, a_event, a_data);
	}

	auto* player = RE::PlayerCharacter::GetSingleton();
	if (!player) return func(a_this, a_event, a_data);

	auto& manager = VCD::Manager::GetSingleton();
	if (player->IsSneaking()) {
		auto standingHeight = manager.GetStandingCapsuleHeight(player);
		if (standingHeight <= 0.0F) {
			standingHeight = player->GetHeight();
		}

		if (!raycast::canStandUp(standingHeight)) {
			return;
		}
	}


	if (a_event->IsUp()) {
	
		// let game process button first so isSneaking() returns valid state
		func(a_this, a_event, a_data);

		const bool gameIsSneaking = player->IsSneaking();

		logger::info("ProcessButton fired, gameIsSneaking={}", gameIsSneaking);

		const auto poseFlags = PoseFixes::PlayerPose(player);
		if (gameIsSneaking) {
			logger::info("player is sneaking, shrinking collision");
		}
		else {
			logger::info("player stood up, restoring collision");
		}
		manager.FixSneakingPose(player, poseFlags, false);

		return; // already called func above
	}

	return func(a_this, a_event, a_data);
}

void SneakHandlerProcessButton::Install()
{
	const auto dllPath = VCD::GetPluginsDir() / "DynamicCollisionAdjustment.dll";

	// dont install if dynamic collision adjustment is installed
	if (std::filesystem::exists(dllPath)) return;
	
	func = REL::Relocation<std::uintptr_t>(RE::SneakHandler::VTABLE[0]).write_vfunc(0x04, thunk);
	logger::info("process sneak button hook installed");
}
