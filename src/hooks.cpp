
#include "hooks.hpp"
#include "logger.hpp"
#include "manager.hpp"
#include "dynamics.hpp"
#include "settings.hpp"

using namespace hooks; 

void PlayerCharacter_Update::thunk(RE::PlayerCharacter* player, float delta) {

	func(player, delta);

	if (!player) {
		logger::warn("Player is null - can't check cell");
		return;
	}

	Dynamics::Update(player);

	if (Settings::GetSettings().drawCollision) {
		VCD::Manager::GetSingleton().DrawPlayerBumper();
	}


}

void PlayerCharacter_Update::Install()
{
	func = REL::Relocation<std::uintptr_t>(RE::PlayerCharacter::VTABLE[0]).write_vfunc(0xAD, thunk);
	logger::info("Player update hook installed");
}
