
#include "hooks.hpp"
#include "logger.hpp"
#include "manager.hpp"
#include "dynamics.hpp"
#include "settings.hpp"
#include "drawLines.hpp"
#include "raycast.hpp"

using namespace Hook; 

void PlayerUpdate::thunk(RE::PlayerCharacter* player, float delta) {

	func(player, delta);

	if (!player) {
		logger::warn("Player is null, cannot check cell");
		return;
	}

	Dynamics::Update(player);

	if (Settings::GetSettings().drawCollision) {
		DebugAPI_IMPL::DebugAPI_Ext::DrawPlayerBumper();
		DebugAPI_IMPL::DebugAPI::GetSingleton()->Update();
	}
}

void PlayerUpdate::Install()
{
	func = REL::Relocation<std::uintptr_t>(RE::PlayerCharacter::VTABLE[0]).write_vfunc(0xAD, thunk);
	logger::info("Player update hook installed");
}

bool SneakHandlerCanProcess::thunk(RE::InputEvent* a_event) {
	
	if (!raycast::canStandUp) {
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