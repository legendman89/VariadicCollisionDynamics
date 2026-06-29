#include "dynamics.hpp"
#include "helper.hpp"
#include "logger.hpp"
#include "posefixes.hpp"
#include "settings.hpp"

#include <algorithm>
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

	bool HasPoseFixedNPCHandle(const RE::Actor* a_actor)
	{
		if (!a_actor) {
			return false;
		}

		auto& state = GetNPCDynamicsState();
		for (auto& handle : state.poseFixedActors) {
			auto actorPtr = handle.get();
			if (actorPtr.get() == a_actor) {
				return true;
			}
		}

		return false;
	}

	void TrackPoseFixedNPC(RE::Actor* a_actor)
	{
		if (!a_actor || HasPoseFixedNPCHandle(a_actor)) {
			return;
		}

		GetNPCDynamicsState().poseFixedActors.push_back(a_actor->CreateRefHandle());
	}

	void RestoreNPCPoseFixes()
	{
		auto& state = GetNPCDynamicsState();
		auto& manager = VCD::Manager::GetSingleton();
		for (auto& handle : state.poseFixedActors) {
			auto actorPtr = handle.get();
			auto* actor = actorPtr.get();
			if (actor) {
				manager.FixSittingPose(actor, {}, false);
			}
		}

		state.poseFixedActors.clear();
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
		const auto poseFlags = PoseFixes::NPCPose(a_actor);
		auto* actorState = FindNPCPresetState(formID);
		auto* controller = a_actor->GetCharController();
		const auto* name = a_actor->GetDisplayFullName();

		if (actorState && actorState->controller != controller) {
			VCD::Manager::GetSingleton().ClearActorRuntimeState(formID);
		}
		if (poseFlags.isSitting && !actorState) {
			logger::debug("NPC: {} [{:08X}] first tracked while sitting", name ? name : "Actor", formID);
		}
		if (actorState && actorState->controller == controller && actorState->preset == a_preset && std::strcmp(actorState->stateName, a_stateName) == 0 && actorState->poseFlags == poseFlags) {
			return true;
		}

		auto& manager = VCD::Manager::GetSingleton();
		const auto* collisionData = GetNPCCollisionData(formID, a_preset);
		if (!collisionData || !manager.SetCollisionData(a_actor, *collisionData, a_preset, VCD::PresetName(a_preset), poseFlags, false)) {
			return false;
		}

		// Cache actor state.
		if (!actorState) {
			auto& state = GetNPCDynamicsState();
			state.actors.push_back({});
			actorState = &state.actors.back();
		}
		actorState->formID = formID;
		actorState->actor = a_actor->CreateRefHandle();
		actorState->controller = controller;
		actorState->preset = a_preset;
		actorState->stateName = a_stateName;
		actorState->poseFlags = poseFlags;

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

	bool UpdateNPCPoseFixHandle(const RE::ActorHandle& a_handle, const RE::PlayerCharacter* a_player, const float& a_radiusSquared)
	{
		auto actorPtr = a_handle.get();
		auto* actor = actorPtr.get();
		if (!CanApplyNPCDynamics(actor, a_player, a_radiusSquared)) {
			return false;
		}

		if (!HasNearbyNPCHandle(actor)) {
			GetNPCDynamicsState().nearbyActors.push_back(a_handle);
		}

		const auto poseFlags = PoseFixes::NPCPose(actor);
		const auto applied = VCD::Manager::GetSingleton().FixSittingPose(actor, poseFlags, false);
		if (applied && poseFlags.isSitting) {
			TrackPoseFixedNPC(actor);
		}

		return applied;
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

	bool UpdateNPCPoseFixHandles(const RE::BSTArray<RE::ActorHandle>& a_handles, const RE::PlayerCharacter* a_player, const float& a_radiusSquared, const int& a_limit)
	{
		for (auto& handle : a_handles) {
			if (a_limit > 0 && static_cast<int>(GetNPCDynamicsState().nearbyActors.size()) >= a_limit) {
				return true;
			}

			UpdateNPCPoseFixHandle(handle, a_player, a_radiusSquared);
		}

		return false;
	}

	void RetireDistantNPCStates()
	{
		auto& state = GetNPCDynamicsState();
		auto& manager = VCD::Manager::GetSingleton();
		std::erase_if(state.actors, [&](const NPCPresetState& a_actorState) {
			const auto nearby = std::any_of(state.nearbyActors.begin(), state.nearbyActors.end(), [&](const RE::ActorHandle& a_handle) {
				auto actorPtr = a_handle.get();
				auto* actor = actorPtr.get();
				return actor && actor->GetFormID() == a_actorState.formID;
			});
			if (!nearby) {
				manager.ClearActorTransientState(a_actorState.formID);
			}
			return !nearby;
		});
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

		VCD::Manager::GetSingleton().SetPreset(a_player, a_preset, state.poseFlags, false);
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
		const auto poseFlags = PoseFixes::PlayerPose(a_player);
		if (!a_force && !preview.active && state.applied && state.current == a_preset && std::strcmp(state.stateName, a_state) == 0 && state.poseFlags == poseFlags) {
			return true;
		}

		auto& manager = VCD::Manager::GetSingleton();
		if (!manager.SetPreset(a_player, a_preset, poseFlags, true)) {
			return false;
		}

		logger::info("Player: {} -> {}, applied preset [{}]", state.stateName, a_state, VCD::PresetName(a_preset));

		// Cache player state.
		preview.active = false;
		preview.restorePending = false;
		state.current = a_preset;
		state.applied = true;
		state.stateName = a_state;
		state.poseFlags = poseFlags;
		StartTransitionRetry(state, a_state);
		return true;
	}

	void ApplyCameraPreset(const VCD::Preset& a_preset)
	{
		auto manger = VCD::Manager::GetSingleton(); 

		auto& state = GetPresetState();
		if (!Settings::GetSettings().enableCameraDynamics) {
			RestoreCameraToVanilla();
			return;
		}

		const auto* presetConfig = VCD::Manager::GetSingleton().GetPresetConfig(a_preset);
		if (!presetConfig) return;
		const auto* collisionData = Settings::GetCameraPresetOverride(a_preset);
		if (!collisionData) {
			collisionData = &presetConfig->data;
		}

		manger.SetCameraCollisionData(*collisionData);
		
		state.currentCamera = a_preset;  
		logger::info("set camera state to {}", VCD::PresetName(a_preset));
	}

	void ApplyCameraCollisionRadius(float a_radius)
	{

		auto* setting = RE::INISettingCollection::GetSingleton()->GetSetting("fCameraCasterSize:Camera");
		if (setting) {
			logger::info("fCameraCasterSize current = {}", setting->data.f);

			// no need to convert to hkp scale for some reason
			setting->data.f = a_radius; 
		}
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

		const auto poseFlags = PoseFixes::PlayerPose(a_player);
		if (!VCD::Manager::GetSingleton().SetPreset(a_player, a_preset, poseFlags, true)) {
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
		const auto poseFlags = PoseFixes::NPCPose(a_actor);
		if (!collisionData || !manager.SetCollisionData(a_actor, *collisionData, a_preset, VCD::PresetName(a_preset), poseFlags, false)) {
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
		const auto preset = Settings::GetSettings().enableNPCDynamics ? GetNPCPreset(actor, stateName) : VCD::Preset::kVanilla;
		const auto* collisionData = Settings::GetSettings().enableNPCDynamics ? GetNPCCollisionData(actor->GetFormID(), preset) : nullptr;
		if (!collisionData) {
			const auto* defaultPresetConfig = manager.GetDefaultPresetConfig(preset);
			collisionData = defaultPresetConfig ? &defaultPresetConfig->data : nullptr;
		}
		const auto poseFlags = PoseFixes::NPCPose(actor);
		if (collisionData) {
			manager.SetCollisionData(actor, *collisionData, preset, VCD::PresetName(preset), poseFlags, false);
		}
		if (auto* actorState = FindNPCPresetState(actor->GetFormID())) {
			actorState->controller = actor->GetCharController();
			actorState->preset = preset;
			actorState->stateName = stateName;
			actorState->poseFlags = poseFlags;
		}
	}

	void RestoreNPCsToVanilla()
	{
		auto& state = GetNPCDynamicsState();
		auto& manager = VCD::Manager::GetSingleton();
		const auto* defaultPresetConfig = manager.GetDefaultPresetConfig(VCD::Preset::kVanilla);
		for (auto& actorState : state.actors) {
			auto actorPtr = actorState.actor.get();
			auto* actor = actorPtr.get();
			if (actor && defaultPresetConfig) {
				const auto poseFlags = PoseFixes::NPCPose(actor);
				manager.SetCollisionData(actor, defaultPresetConfig->data, VCD::Preset::kVanilla, VCD::PresetName(VCD::Preset::kVanilla), poseFlags, false);
			}
		}

		state.actors.clear();
		state.nearbyActors.clear();
	}

	void RestoreCameraToVanilla() {

		ApplyCameraCollisionRadius(15.0f);
		GetPresetState().currentCamera = VCD::Preset::kCameraVanilla;
	}

	void SchedulePostLoadApply()
	{
		auto& state = GetPresetState();
		state.postLoadApplyPending = true;
		state.postLoadTimerStarted = false;
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

		// Ordered by priority: Werewolf > Vampire Lord > Combat > Environment.
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

	void ClearRuntimeState()
	{
		GetPresetState() = {};
		GetPreviewState() = {};
		GetNPCPreviewState() = {};
		GetNPCDynamicsState() = {};
		VCD::Manager::GetSingleton().ClearRuntimeState();
	}

	void RemapDeletedPreset(const VCD::Preset& a_preset)
	{
		auto& config = GetConfig();
#define PRESET_STATE2REMAP(S, D) VCD::RemapPresetAfterDeletion(config.S, a_preset);
		FOREACH_PRESET_STATE(PRESET_STATE2REMAP)
		ClearRuntimeState();
	}

	void Update(const RE::PlayerCharacter* a_player)
	{
		if (!a_player) {
			return;
		}

		auto* controller = a_player->GetCharController();
		if (!controller) {
			return;
		}

		auto& state = GetPresetState();
		if (state.controller && state.controller != controller) {
			ClearRuntimeState();
			logger::debug("Player controller changed; runtime state cleared");
		}
		state.controller = controller;

		bool forceApply = false;
		if (state.postLoadApplyPending) {
			const auto now = std::chrono::steady_clock::now();
			if (!state.postLoadTimerStarted) {
				state.postLoadTimerStarted = true;
				state.postLoadApplyAt = now + std::chrono::seconds(1);
				logger::debug("Post-load preset timer started");
				return;
			}
			if (now < state.postLoadApplyAt) {
				return;
			}
			forceApply = true;
		}

		auto* cell = a_player->GetParentCell();
		if (!cell) {
			return;
		}

		auto& preview = GetPreviewState();
		if (preview.restorePending && std::chrono::steady_clock::now() >= preview.restoreAt) {
			RestorePresetPreview(a_player);
		}

		const auto poseFlags = PoseFixes::PlayerPose(a_player);
		const auto* stateName = GetCellStateName(cell);
		auto preset = GetCellPreset(cell);
		auto cameraPreset = GetCellCameraPreset(cell);

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

		if (!forceApply && state.lastCell == cell && state.applied && state.current == preset && std::strcmp(state.stateName, stateName) == 0 && state.poseFlags == poseFlags) {
			if (Settings::GetSettings().enableCameraDynamics && state.currentCamera != VCD::Preset::kCameraDialogue && state.currentCamera != cameraPreset) {
				ApplyCameraPreset(cameraPreset);
			}
			RetryTransitionPreset(a_player, preset, stateName);
			return;
		}

		state.lastCell = cell;
		if (ApplyPreset(a_player, preset, stateName, forceApply) && forceApply) {
			state.postLoadApplyPending = false;
			logger::debug("Post-load preset timer expired");
		}

		if (Settings::GetSettings().enableCameraDynamics && state.currentCamera != VCD::Preset::kCameraDialogue && (forceApply || state.currentCamera != cameraPreset)) {
			ApplyCameraPreset(cameraPreset);
		}
		return;
	}

	void UpdateNPCs(const RE::PlayerCharacter* a_player)
	{
		if (!a_player) {
			return;
		}

		auto& state = GetNPCDynamicsState();
		const auto now = std::chrono::steady_clock::now();
		const auto& settings = Settings::GetSettings();

		if (!Settings::GetSettings().enableNPCDynamics) {
			if (!state.actors.empty()) {
				RestoreNPCsToVanilla();
			}

			if (!settings.fixNPCSitting) {
				if (!state.poseFixedActors.empty()) {
					RestoreNPCPoseFixes();
				}
				return;
			}

			if (now < state.nextScan) {
				return;
			}

			const auto interval = std::chrono::duration<float>(settings.nearbyActorScanInterval);
			state.nextScan = now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(interval);
			state.nearbyActors.clear();

			auto* processLists = RE::ProcessLists::GetSingleton();
			if (!processLists) {
				return;
			}

			const auto radiusSquared = settings.nearbyActorDrawRadius * settings.nearbyActorDrawRadius;
			if (UpdateNPCPoseFixHandles(processLists->highActorHandles, a_player, radiusSquared, settings.nearbyActorDrawLimit) ||
				UpdateNPCPoseFixHandles(processLists->middleHighActorHandles, a_player, radiusSquared, settings.nearbyActorDrawLimit) ||
				UpdateNPCPoseFixHandles(processLists->middleLowActorHandles, a_player, radiusSquared, settings.nearbyActorDrawLimit)) {
				return;
			}
			return;
		}

		if (!state.poseFixedActors.empty()) {
			state.poseFixedActors.clear();
		}

		if (now < state.nextScan) {
			return;
		}

		const auto interval = std::chrono::duration<float>(settings.nearbyActorScanInterval);
		state.nextScan = now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(interval);
		state.nearbyActors.clear();

		// Credit: Truman for his idea to check actor handles in ProcessLists.
		auto* processLists = RE::ProcessLists::GetSingleton();
		if (!processLists) {
			return;
		}

		const auto radiusSquared = settings.nearbyActorDrawRadius * settings.nearbyActorDrawRadius;
		if (!UpdateNPCHandles(processLists->highActorHandles, a_player, radiusSquared, settings.nearbyActorDrawLimit) &&
			!UpdateNPCHandles(processLists->middleHighActorHandles, a_player, radiusSquared, settings.nearbyActorDrawLimit)) {
			UpdateNPCHandles(processLists->middleLowActorHandles, a_player, radiusSquared, settings.nearbyActorDrawLimit);
		}

		RetireDistantNPCStates();
	}

}

#undef PRESET_STATE2REMAP
