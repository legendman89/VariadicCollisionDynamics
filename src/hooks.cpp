
#include "hooks.hpp"
#include "logger.hpp"
#include "globals.hpp"
#include "manager.hpp"

using namespace hooks; 

void PlayerCharacter_Update::thunk(RE::PlayerCharacter* player, float delta) {

	func(player, delta);

	if (!player) {
		logger::warn("player is null cant check cell");
		return;
	}

	if (globals::bDrawCharacterBumper) {
		logger::info("Drawing bumper");
		VCD::Manager::GetSingleton().DrawPlayerBumper();
	}


}

void PlayerCharacter_Update::Install()
{
	func = REL::Relocation<std::uintptr_t>(RE::PlayerCharacter::VTABLE[0])
		.write_vfunc(0xAD, thunk);
	logger::info("player chracter update hook installed");
}