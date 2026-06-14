#include "dynamics.hpp"
#include "logger.hpp"

#include <chrono>
#include <cstring>
#include <string_view>

namespace Dynamics {

	constexpr auto kTransitionRetryDuration = std::chrono::duration<float>(2.0F);
	constexpr auto kTransitionRetryInterval = std::chrono::duration<float>(0.15F);

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

	bool IsWerewolf(const RE::Actor* a_actor)
	{
		if (!a_actor) {
			return false;
		}

		const auto* race = a_actor->GetRace();
		if (!race) {
			return false;
		}

		const auto* editorID = race->GetFormEditorID();
		return editorID && std::string_view(editorID) == "WerewolfBeastRace";
	}

	bool IsVampireLord(const RE::Actor* a_actor)
	{
		if (!a_actor) {
			return false;
		}

		const auto* race = a_actor->GetRace();
		if (!race) {
			return false;
		}

		const auto* editorID = race->GetFormEditorID();
		return editorID && std::string_view(editorID) == "DLC1VampireBeastRace";
	}

	bool IsTransformationState(const char* a_state)
	{
		return std::strcmp(a_state, "werewolf") == 0 || std::strcmp(a_state, "vampireLord") == 0;
	}

	void StartTransitionRetry(PresetState& a_state, const char* a_stateName)
	{
		if (!IsTransformationState(a_stateName)) {
			a_state.transitionRetryActive = false;
			return;
		}

		const auto now = std::chrono::steady_clock::now();
		a_state.transitionRetryActive = true;
		a_state.transitionRetryUntil = now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(kTransitionRetryDuration);
		a_state.nextTransitionRetry = now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(kTransitionRetryInterval);
	}

	bool RetryTransitionPreset(const RE::PlayerCharacter* a_player, const VCD::Preset& a_preset, const char* a_stateName)
	{
		auto& state = GetPresetState();
		if (!state.transitionRetryActive || !state.applied || state.current != a_preset || std::strcmp(state.stateName, a_stateName) != 0) {
			return false;
		}

		const auto now = std::chrono::steady_clock::now();
		if (now >= state.transitionRetryUntil) {
			state.transitionRetryActive = false;
			return false;
		}

		if (now < state.nextTransitionRetry) {
			return true;
		}

		VCD::Manager::GetSingleton().SetPreset(a_player, a_preset, false);
		state.nextTransitionRetry = now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(kTransitionRetryInterval);
		return true;
	}

	bool ApplyPreset(const RE::PlayerCharacter* a_player, const VCD::Preset& a_preset, const char* a_state, const bool& a_force)
	{
		if (!a_player) {
			return false;
		}

		auto& state = GetPresetState();
		auto& preview = GetPreviewState();
		if (!a_force && !preview.active && state.applied && state.current == a_preset && std::strcmp(state.stateName, a_state) == 0) {
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
		StartTransitionRetry(state, a_state);
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

		if (IsWerewolf(a_player)) {
			ApplyPreset(a_player, GetConfig().werewolf, "werewolf", true);
		}
		else if (IsVampireLord(a_player)) {
			ApplyPreset(a_player, GetConfig().vampireLord, "vampireLord", true);
		}
		else if (a_player->IsInCombat()) {
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
		const auto* stateName = GetCellStateName(cell);
		auto preset = GetCellPreset(cell);

		if (IsWerewolf(a_player)) {
			stateName = "werewolf";
			preset = GetConfig().werewolf;
		}
		else if (IsVampireLord(a_player)) {
			stateName = "vampireLord";
			preset = GetConfig().vampireLord;
		}
		else if (a_player->IsInCombat()) {
			stateName = "combat";
			preset = GetConfig().combat;
		}

		if (state.lastCell == cell && state.applied && state.current == preset && std::strcmp(state.stateName, stateName) == 0) {
			RetryTransitionPreset(a_player, preset, stateName);
			return;
		}

		state.lastCell = cell;
		ApplyPreset(a_player, preset, stateName);
	}

}
