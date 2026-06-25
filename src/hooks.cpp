
#include "draw.hpp"
#include "hooks.hpp"
#include "logger.hpp"
#include "manager.hpp"
#include "dynamics.hpp"
#include "settings.hpp"
#include "raycast.hpp"
#include "posefixes.hpp"
#include <CLibUtilsQTR/DrawDebug.hpp>

using namespace Hook; 
using namespace DebugAPI_IMPL;

namespace SneakGate {
	std::atomic_bool blockSneak = false;
}

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
		if (playerIsSneaking) {
			auto& manager = VCD::Manager::GetSingleton();
			const auto sittingFlags = PoseFixes::PlayerSitting(player); 
			manager.FixSneakingPose(player, true, sittingFlags, false);
		}
	}

	// block if player is sneaking. (we should think of situations where this might break stuff) 
	if (!player->IsSneaking()) {
		Dynamics::Update(player);
	}

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

bool SneakHandlerCanProcess::thunk(RE::SneakHandler* a_this, RE::InputEvent* a_event) {
	if (!a_this || !a_event) return func(a_this, a_event);

	auto* player = RE::PlayerCharacter::GetSingleton();
	if (!player) return func(a_this, a_event);

	auto& manager = VCD::Manager::GetSingleton();
	const auto sittingFlags = PoseFixes::PlayerSitting(player);

//  return if not sneaking 
    if (!player->IsSneaking())
        return func(a_this, a_event);

	auto standingHeight = manager.GetStandingCapsuleHeight(player);
	if (standingHeight <= 0.0F) {
		standingHeight = player->GetHeight();
	}

    if (!raycast::canStandUp(standingHeight))
    {
        SneakGate::blockSneak.store(true); 
        return false;
    }
    else {
        SneakGate::blockSneak.store(false);
    }

    return func(a_this, a_event);
}

void SneakHandlerCanProcess::Install()
{
	constexpr auto dllPath = "Data/SKSE/Plugins/DynamicCollisionAdjustment.dll";
	// dont install if dynamic collision adjustment is installed
	if (std::filesystem::exists(dllPath)) return;
	func = REL::Relocation<std::uintptr_t>(RE::SneakHandler::VTABLE[0]).write_vfunc(0x01, thunk);
	logger::info("Can Process Sneak hook installed");
}

// isUp() called only once per sneak key press
void SneakHandlerProcessButton::thunk(
	RE::SneakHandler* a_this,
	RE::ButtonEvent* a_event,
	RE::PlayerControlsData* a_data)
{
	if (!a_this || !a_event || !a_data)
		return func(a_this, a_event, a_data);

	if (a_event->IsUp() && !SneakGate::blockSneak.load()) {

		// let game process button first so isSneaking() returns valid state
		func(a_this, a_event, a_data);

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) return;

		const bool gameIsSneaking = player->IsSneaking();

		logger::info("ProcessButton fired, gameIsSneaking={}", gameIsSneaking);

		auto& manager = VCD::Manager::GetSingleton();
		const auto sittingFlags = PoseFixes::PlayerSitting(player);

		if (gameIsSneaking) {
			logger::info("player is sneaking, shrinking collision");
			manager.FixSneakingPose(player, true, sittingFlags, false);
		}
		else {
			logger::info("player stood up, restoring collision");
			manager.FixSneakingPose(player, false, sittingFlags, false);
		}

		return; // already called func above
	}

	return func(a_this, a_event, a_data);
}

void SneakHandlerProcessButton::Install()
{
	constexpr auto dllPath = "Data/SKSE/Plugins/DynamicCollisionAdjustment.dll";

	// dont install if dynamic collision adjustment is installed
	if (std::filesystem::exists(dllPath)) return;
	func = REL::Relocation<std::uintptr_t>(RE::SneakHandler::VTABLE[0]).write_vfunc(0x04, thunk);
	logger::info("process sneak button hook installed");
}