
#include "draw.hpp"
#include "hooks.hpp"
#include "logger.hpp"
#include "manager.hpp"
#include "dynamics.hpp"
#include "settings.hpp"
#include "raycast.hpp"

#include <CLibUtilsQTR/DrawDebug.hpp>

using namespace Hook; 
using namespace DebugAPI_IMPL;

void PlayerUpdate::thunk(RE::PlayerCharacter* player, float delta) {

	func(player, delta);

	if (!player) {
		logger::warn("Player is null, cannot update.");
		return;
	}

	Dynamics::Update(player);
	Dynamics::UpdateNPCs(player);

	const auto& settings = Settings::GetSettings();
	if (settings.drawCollision || settings.drawNearbyActors) {
		DebugAPI_IMPL::DebugAPI::GetSingleton()->LinesToDraw.clear();
	}

	if (settings.drawCollision) {
		Draw::DrawPlayerBumper();
	}

	if (settings.drawNearbyActors) {
		Draw::DrawNearbyActorBumpers();
	}

	if (settings.drawCollision || settings.drawNearbyActors) {
		DebugAPI_IMPL::DebugAPI::GetSingleton()->Update();
	}
}

void PlayerUpdate::Install()
{
	func = REL::Relocation<std::uintptr_t>(RE::PlayerCharacter::VTABLE[0]).write_vfunc(0xAD, thunk);
	logger::info("Player update hook installed");
}

bool SneakHandlerCanProcess::thunk(RE::InputEvent* a_event) {
	
	if (!raycast::canStandUp()) {
		return false; 
	}

	else {
		return func(a_event);
	} 
}

void SneakHandlerCanProcess::Install()
{
	func = REL::Relocation<std::uintptr_t>(RE::SneakHandler::VTABLE[0]).write_vfunc(0x01, thunk);
	logger::info("Can Process Sneak hook installed");
}
