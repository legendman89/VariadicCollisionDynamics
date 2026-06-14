
#include "hooks.hpp"
#include "logger.hpp"
#include "manager.hpp"
#include "dynamics.hpp"
#include "settings.hpp"
#include "drawLines.hpp"

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
