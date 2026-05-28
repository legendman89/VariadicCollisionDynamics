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

		auto& collisionNode =
			VCD::Manager::GetSingleton().GetPresetMeshes()
			[VCD::ToUnderlying(VCD::Preset::kPersonalSpace)].spCollisionObject;

		if (!collisionNode) return RE::BSEventNotifyControl::kContinue;

		auto* sp = collisionNode.get();
		if (!sp || !sp->body)
			return RE::BSEventNotifyControl::kContinue;

		auto* hkWorldObj = static_cast<RE::hkpWorldObject*>(
			collisionNode->body->referencedObject.get()
			);

		if (!hkWorldObj || !hkWorldObj->collidable.shape)
			return RE::BSEventNotifyControl::kContinue;

		auto* shape = hkWorldObj->collidable.shape;

		auto playerController = player->GetCharController(); 

		if (auto proxyController = skyrim_cast<RE::bhkCharProxyController*>(playerController)) {

			logger::debug("Grabbed Character Controller grabbing proxy now");

			auto proxy = static_cast<RE::hkpCharacterProxy*>(
				proxyController->proxy.referencedObject.get()
				);

			if (proxy) {

				logger::debug("Grabbed proxy setting collision now");

				logger::info("Shape ptr before swap: {:p}", (void*)proxy->shapePhantom->collidable.shape);

				proxy->shapePhantom->SetShape(
					shape
				);

				logger::info("Shape ptr after swap: {:p}", (void*)proxy->shapePhantom->collidable.shape);
			}

			else {
				logger::error("no Proxy Cant Change Players Collision");
			}
		}

		else {
			logger::error("no Proxy Controller Cant Change Players Collision");
		}


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