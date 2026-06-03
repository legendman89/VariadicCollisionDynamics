#include "eventSinks.hpp"
#include "logger.hpp"
#include "manager.hpp"

namespace EventSinks {

	// normally bgsActorCelLEvent runs for every NPC but since we made the player the event source,
	// this only runs for player character, ideal for changing collision base on interiors / or location type keywords
	RE::BSEventNotifyControl PlayerCellEvent::ProcessEvent(const RE::BGSActorCellEvent* event,
		RE::BSTEventSource<RE::BGSActorCellEvent>*) {


		if (!event || event->flags == RE::BGSActorCellEvent::CellFlag::kLeave) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto player = RE::PlayerCharacter::GetSingleton();

		if (!player) return RE::BSEventNotifyControl::kContinue;

		auto cell = RE::TESForm::LookupByID<RE::TESObjectCELL>(event->cellID);
		if (!cell) {
			return RE::BSEventNotifyControl::kContinue;
		}

		// can change collision based on location keywords or check if player went from interior to exterior or vice versa

		//globals::currentCellIsInterior = cell->IsInteriorCell();

		//if (globals::lastCellWasInterior != globals::currentCellIsInterior) {

		//}
		//globals::lastCellWasInterior = globals::currentCellIsInterior;

		return RE::BSEventNotifyControl::kContinue;
	}

	// seems to run anytime a npcs combat state changes, fired for every npc
	RE::BSEventNotifyControl CombatEventSink::ProcessEvent(const RE::TESCombatEvent* event,
		RE::BSTEventSource <RE::TESCombatEvent>*) {

		if (!event) {
			return RE::BSEventNotifyControl::kContinue;
		}

		logger::debug("combat event fired");

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto manager = VCD::Manager::GetSingleton();

		// we should prolly allow user to decide what preset they want for combat
		manager.SetPreset(player, VCD::Preset::kCompact);

		return RE::BSEventNotifyControl::kContinue;
	}

	void InstallEventSinks() {
		PlayerCellEvent::RegisterEventSink();
		CombatEventSink::RegisterEventSink();
	}

	void CombatEventSink::RegisterEventSink() {
		auto* eventSink = CombatEventSink::GetSingleton();
		auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
		eventSourceHolder->AddEventSink<RE::TESCombatEvent>(eventSink);
		logger::info("CombatEventSink sink registered");
	}

	void PlayerCellEvent::RegisterEventSink() {
		if (auto* player = RE::PlayerCharacter::GetSingleton()) {
			player->AsBGSActorCellEventSource()->AddEventSink(PlayerCellEvent::GetSingleton());
			logger::info("BGSActorCellEvent sink registered");
		}
	}
}