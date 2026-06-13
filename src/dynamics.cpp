#include "dynamics.hpp"
#include "logger.hpp"

namespace Dynamics {

	const char* PresetName(const VCD::Preset& a_preset)
	{
		switch (a_preset) {
		case VCD::Preset::kVanillaLike:
			return "Vanilla-like";
		case VCD::Preset::kPersonalSpace:
			return "Personal Space";
		case VCD::Preset::kCompact:
			return "Compact";
		case VCD::Preset::kBulky:
			return "Bulky";
		default:
			return "Unknown";
		}
	}

	VCD::Preset GetCellPreset(const RE::TESObjectCELL* a_cell)
	{
		auto& config = GetConfig();
		if (!a_cell) {
			return config.neutral;
		}

		return a_cell->IsInteriorCell() ? config.indoor : config.outdoor;
	}

	const char* GetCellStateName(const RE::TESObjectCELL* a_cell)
	{
		if (!a_cell) {
			return "unknown";
		}

		return a_cell->IsInteriorCell() ? "indoor" : "outdoor";
	}

	bool ApplyPreset(const RE::PlayerCharacter* a_player, const VCD::Preset& a_preset, const char* a_state)
	{
		if (!a_player) {
			return false;
		}

		auto& state = GetPresetState();
		if (state.applied && state.current == a_preset) {
			return true;
		}

		auto& manager = VCD::Manager::GetSingleton();
		if (!manager.SetPreset(a_player, a_preset)) {
			return false;
		}

		logger::info("Player: {} -> {}, applied preset [{}]", state.stateName, a_state, PresetName(a_preset));

		state.current = a_preset;
		state.applied = true;
		state.stateName = a_state;
		return true;
	}

	bool ApplyEnvironmentPreset(const RE::PlayerCharacter* a_player)
	{
		if (!a_player) {
			return false;
		}

		auto* cell = a_player->GetParentCell();
		return ApplyPreset(a_player, GetCellPreset(cell), GetCellStateName(cell));
	}

	void Update(const RE::PlayerCharacter* a_player)
	{
		if (!a_player) {
			return;
		}

		if (!VCD::Manager::GetSingleton().AreAllPresetsLoaded()) {
			return;
		}

		auto* cell = a_player->GetParentCell();
		if (!cell) {
			return;
		}

		auto& state = GetPresetState();
		if (state.lastCell == cell) {
			return;
		}

		state.lastCell = cell;

		if (a_player->IsInCombat()) {
			ApplyPreset(a_player, GetConfig().combat, "combat");
		}
		else {
			ApplyPreset(a_player, GetCellPreset(cell), GetCellStateName(cell));
		}
	}

	RE::BSEventNotifyControl PlayerCellEvent::ProcessEvent(const RE::BGSActorCellEvent* event, RE::BSTEventSource<RE::BGSActorCellEvent>*) {


		if (!event || event->flags == RE::BGSActorCellEvent::CellFlag::kLeave) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();

		if (!player) return RE::BSEventNotifyControl::kContinue;

		if (player->IsInCombat()) {
			ApplyPreset(player, GetConfig().combat, "combat");
		}
		else {
			auto* cell = RE::TESForm::LookupByID<RE::TESObjectCELL>(event->cellID);
			ApplyPreset(player, GetCellPreset(cell), GetCellStateName(cell));
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl CombatEventSink::ProcessEvent(const RE::TESCombatEvent* event, RE::BSTEventSource <RE::TESCombatEvent>*) {

		if (!event) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (event->actor.get() != player) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (event->newState == RE::ACTOR_COMBAT_STATE::kCombat) {
			ApplyPreset(player, GetConfig().combat, "combat");
		}
		else {
			ApplyEnvironmentPreset(player);
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	void Install() {
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
