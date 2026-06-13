#include "dynamics.hpp"
#include "logger.hpp"

#include <chrono>

namespace Dynamics {

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

	bool IsPresetCurrent(const VCD::Preset& a_preset)
	{
		const auto& state = GetPresetState();
		return state.applied && state.current == a_preset;
	}

	bool IsPresetPreviewed(const VCD::Preset& a_preset)
	{
		const auto& preview = GetPreviewState();
		return preview.active && preview.preset == a_preset;
	}

	bool ApplyPreset(const RE::PlayerCharacter* a_player, const VCD::Preset& a_preset, const char* a_state, const bool& a_force)
	{
		if (!a_player) {
			return false;
		}

		auto& state = GetPresetState();
		auto& preview = GetPreviewState();
		if (!a_force && !preview.active && state.applied && state.current == a_preset) {
			return true;
		}

		auto& manager = VCD::Manager::GetSingleton();
		if (!manager.SetPreset(a_player, a_preset)) {
			return false;
		}

		logger::info("Player: {} -> {}, applied preset [{}]", state.stateName, a_state, VCD::PresetName(a_preset));

		preview.active = false;
		preview.restorePending = false;
		state.current = a_preset;
		state.applied = true;
		state.stateName = a_state;
		return true;
	}

	bool ApplyEnvironmentPreset(const RE::PlayerCharacter* a_player, const bool& a_force)
	{
		if (!a_player) {
			return false;
		}

		auto* cell = a_player->GetParentCell();
		return ApplyPreset(a_player, GetCellPreset(cell), GetCellStateName(cell), a_force);
	}

	bool StartPresetPreview(const RE::PlayerCharacter* a_player, const VCD::Preset& a_preset)
	{
		if (!a_player) {
			return false;
		}

		if (!VCD::Manager::GetSingleton().SetPreset(a_player, a_preset)) {
			return false;
		}

		auto& preview = GetPreviewState();
		preview.active = true;
		preview.restorePending = false;
		preview.preset = a_preset;
		logger::info("Player: previewing preset [{}]", VCD::PresetName(a_preset));
		return true;
	}

	void SchedulePreviewRestore(const float& a_delaySeconds)
	{
		auto& preview = GetPreviewState();
		if (!preview.active) {
			return;
		}

		const auto delay = std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<float>(a_delaySeconds));
		preview.restorePending = true;
		preview.restoreAt = std::chrono::steady_clock::now() + delay;
	}

	void RestorePresetPreview(const RE::PlayerCharacter* a_player)
	{
		auto& preview = GetPreviewState();
		if (!preview.active && !preview.restorePending) {
			return;
		}

		preview.active = false;
		preview.restorePending = false;

		if (!a_player) {
			return;
		}

		if (a_player->IsInCombat()) {
			ApplyPreset(a_player, GetConfig().combat, "combat", true);
		}
		else {
			ApplyEnvironmentPreset(a_player, true);
		}
	}

	void Update(const RE::PlayerCharacter* a_player)
	{
		if (!a_player) {
			return;
		}

		if (!VCD::Manager::GetSingleton().AreAllPresetsLoaded()) {
			return;
		}

		auto& preview = GetPreviewState();
		if (preview.restorePending && std::chrono::steady_clock::now() >= preview.restoreAt) {
			RestorePresetPreview(a_player);
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
