#include "dynamics.hpp"
#include "logger.hpp"
#include "settings.hpp"

#include <chrono>
#include <cstring>
#include <string_view>

namespace Dynamics {

	constexpr auto kTransitionRetryDuration = std::chrono::duration<float>(2.0F);
	constexpr auto kTransitionRetryInterval = std::chrono::duration<float>(0.15F);

	bool CanApplyNPCDynamics(RE::Actor* a_actor, const RE::PlayerCharacter* a_player, const float& a_radiusSquared)
	{
		if (!a_actor || !a_player || a_actor == a_player) {
			return false;
		}

		if (a_actor->IsDead() || a_actor->IsDisabled() || !a_actor->Get3D() || (!a_actor->IsGuard() && !a_actor->HasKeywordString("ActorTypeNPC"))) {
			return false;
		}

		const auto* actorCell = a_actor->GetParentCell();
		const auto* playerCell = a_player->GetParentCell();
		if (!actorCell || !playerCell || ((actorCell->IsInteriorCell() || playerCell->IsInteriorCell()) && actorCell != playerCell)) {
			return false;
		}

		const auto x = a_actor->GetPosition().x - a_player->GetPosition().x;
		const auto y = a_actor->GetPosition().y - a_player->GetPosition().y;
		const auto z = a_actor->GetPosition().z - a_player->GetPosition().z;
		return ((x * x) + (y * y) + (z * z)) <= a_radiusSquared;
	}

	VCD::Preset GetNPCPreset(const RE::Actor* a_actor, const char*& a_stateName)
	{
		auto& config = GetConfig();
		const auto isCombat = a_actor && a_actor->IsInCombat();
		const auto isGuard = a_actor && a_actor->IsGuard();

		if (isGuard && isCombat) {
			a_stateName = "guardCombat";
			return config.guardCombat;
		}

		if (isGuard) {
			a_stateName = "guardNeutral";
			return config.guardNeutral;
		}

		if (isCombat) {
			a_stateName = "npcCombat";
			return config.npcCombat;
		}

		a_stateName = "npcNeutral";
		return config.npcNeutral;
	}

	NPCPresetState* FindNPCPresetState(const RE::FormID& a_formID)
	{
		auto& state = GetNPCDynamicsState();
		for (auto& actorState : state.actors) {
			if (actorState.formID == a_formID) {
				return &actorState;
			}
		}

		return nullptr;
	}

	bool HasNearbyNPCHandle(const RE::Actor* a_actor)
	{
		if (!a_actor) {
			return false;
		}

		auto& state = GetNPCDynamicsState();
		for (auto& handle : state.nearbyActors) {
			auto actorPtr = handle.get();
			if (actorPtr.get() == a_actor) {
				return true;
			}
		}

		return false;
	}

	const VCD::CollisionData* GetNPCCollisionData(const RE::FormID& a_formID, const VCD::Preset& a_preset)
	{
		if (const auto* actorOverride = Settings::GetNPCActorPresetOverride(a_formID, a_preset)) {
			return actorOverride;
		}

		if (const auto* npcOverride = Settings::GetNPCPresetOverride(a_preset)) {
			return npcOverride;
		}

		const auto* defaultPresetConfig = VCD::Manager::GetSingleton().GetDefaultPresetConfig(a_preset);
		return defaultPresetConfig ? &defaultPresetConfig->data : nullptr;
	}

	bool ApplyNPCPreset(RE::Actor* a_actor, const VCD::Preset& a_preset, const char* a_stateName)
	{
		if (!a_actor || GetNPCPreviewState().active) {
			return false;
		}

		const auto formID = a_actor->GetFormID();
		auto* actorState = FindNPCPresetState(formID);
		if (actorState && actorState->preset == a_preset && std::strcmp(actorState->stateName, a_stateName) == 0) {
			return true;
		}

		auto& manager = VCD::Manager::GetSingleton();
		const auto* collisionData = GetNPCCollisionData(formID, a_preset);
		if (!collisionData || !manager.SetCollisionData(a_actor, *collisionData, a_preset, VCD::PresetName(a_preset), false)) {
			return false;
		}

		if (!actorState) {
			auto& state = GetNPCDynamicsState();
			state.actors.push_back({});
			actorState = &state.actors.back();
		}

		actorState->formID = formID;
		actorState->actor = a_actor->CreateRefHandle();
		actorState->preset = a_preset;
		actorState->stateName = a_stateName;

		const auto* name = a_actor->GetDisplayFullName();
		logger::debug("NPC: {} [{:08X}] -> {}, applied preset [{}]", name ? name : "Actor", formID, a_stateName, VCD::PresetName(a_preset));
		return true;
	}

	bool UpdateNPCHandle(const RE::ActorHandle& a_handle, const RE::PlayerCharacter* a_player, const float& a_radiusSquared)
	{
		auto actorPtr = a_handle.get();
		auto* actor = actorPtr.get();
		if (!CanApplyNPCDynamics(actor, a_player, a_radiusSquared)) {
			return false;
		}

		if (!HasNearbyNPCHandle(actor)) {
			GetNPCDynamicsState().nearbyActors.push_back(a_handle);
		}

		const char* stateName = "unknown";
		const auto preset = GetNPCPreset(actor, stateName);
		return ApplyNPCPreset(actor, preset, stateName);
	}

	bool UpdateNPCHandles(const RE::BSTArray<RE::ActorHandle>& a_handles, const RE::PlayerCharacter* a_player, const float& a_radiusSquared, const int& a_limit)
	{
		for (auto& handle : a_handles) {
			if (a_limit > 0 && static_cast<int>(GetNPCDynamicsState().nearbyActors.size()) >= a_limit) {
				return true;
			}

			UpdateNPCHandle(handle, a_player, a_radiusSquared);
		}

		return false;
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

	bool StartNPCPresetPreview(RE::Actor* a_actor, const VCD::Preset& a_preset)
	{
		if (!a_actor) {
			return false;
		}

		auto& manager = VCD::Manager::GetSingleton();
		const auto* collisionData = GetNPCCollisionData(a_actor->GetFormID(), a_preset);
		if (!collisionData || !manager.SetCollisionData(a_actor, *collisionData, a_preset, VCD::PresetName(a_preset), false)) {
			return false;
		}

		auto& preview = GetNPCPreviewState();
		preview.active = true;
		preview.actor = a_actor->CreateRefHandle();
		preview.preset = a_preset;

		const auto* name = a_actor->GetDisplayFullName();
		logger::debug("NPC: previewing preset [{}] on {} [{:08X}]", VCD::PresetName(a_preset), name ? name : "Actor", a_actor->GetFormID());
		return true;
	}

	void StopNPCPresetPreview()
	{
		auto& preview = GetNPCPreviewState();
		if (!preview.active) {
			return;
		}

		auto actorPtr = preview.actor.get();
		auto* actor = actorPtr.get();
		preview.active = false;

		if (!actor) {
			return;
		}

		const char* stateName = "unknown";
		auto& manager = VCD::Manager::GetSingleton();
		const auto preset = Settings::GetSettings().enableNPCDynamics ? GetNPCPreset(actor, stateName) : VCD::Preset::kVanillaLike;
		const auto* collisionData = Settings::GetSettings().enableNPCDynamics ? GetNPCCollisionData(actor->GetFormID(), preset) : nullptr;
		if (!collisionData) {
			const auto* defaultPresetConfig = manager.GetDefaultPresetConfig(preset);
			collisionData = defaultPresetConfig ? &defaultPresetConfig->data : nullptr;
		}
		if (collisionData) {
			manager.SetCollisionData(actor, *collisionData, preset, VCD::PresetName(preset), false);
		}
		if (auto* actorState = FindNPCPresetState(actor->GetFormID())) {
			actorState->preset = preset;
			actorState->stateName = stateName;
		}
	}

	void RestoreNPCsToVanillaLike()
	{
		auto& state = GetNPCDynamicsState();
		auto& manager = VCD::Manager::GetSingleton();
		const auto* defaultPresetConfig = manager.GetDefaultPresetConfig(VCD::Preset::kVanillaLike);
		for (auto& actorState : state.actors) {
			auto actorPtr = actorState.actor.get();
			auto* actor = actorPtr.get();
			if (actor && defaultPresetConfig) {
				manager.SetCollisionData(actor, defaultPresetConfig->data, VCD::Preset::kVanillaLike, VCD::PresetName(VCD::Preset::kVanillaLike), false);
			}
		}

		state.actors.clear();
		state.nearbyActors.clear();
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

		// Ordered by priority: Werewolf > Vampire Lord > Combat > Environment
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

	void UpdateNPCs(const RE::PlayerCharacter* a_player)
	{
		if (!a_player) {
			return;
		}

		if (!Settings::GetSettings().enableNPCDynamics) {
			if (!GetNPCDynamicsState().actors.empty()) {
				RestoreNPCsToVanillaLike();
			}
			return;
		}

		if (!VCD::Manager::GetSingleton().AreAllPresetsLoaded()) {
			return;
		}

		auto& state = GetNPCDynamicsState();
		const auto now = std::chrono::steady_clock::now();
		if (now < state.nextScan) {
			return;
		}

		const auto& settings = Settings::GetSettings();
		const auto interval = std::chrono::duration<float>(settings.nearbyActorScanInterval);
		state.nextScan = now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(interval);
		state.nearbyActors.clear();

		auto* processLists = RE::ProcessLists::GetSingleton();
		if (!processLists) {
			return;
		}

		const auto radiusSquared = settings.nearbyActorDrawRadius * settings.nearbyActorDrawRadius;
		if (UpdateNPCHandles(processLists->highActorHandles, a_player, radiusSquared, settings.nearbyActorDrawLimit) ||
			UpdateNPCHandles(processLists->middleHighActorHandles, a_player, radiusSquared, settings.nearbyActorDrawLimit) ||
			UpdateNPCHandles(processLists->middleLowActorHandles, a_player, radiusSquared, settings.nearbyActorDrawLimit)) {
			return;
		}
	}

}
