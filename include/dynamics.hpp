#pragma once

#include "manager.hpp"
#include "preset_states.hpp"
#include "race.hpp"

#include <chrono>
#include <cstring>
#include <string_view>
#include <vector>

namespace Dynamics {

	struct PresetState
	{
		bool applied{ false };

		VCD::Preset current{ VCD::Preset::kVanilla };

		VCD::Preset currentCamera{ VCD::Preset::kCameraVanilla };

		const RE::TESObjectCELL* lastCell{ nullptr };

		const RE::bhkCharacterController* controller{ nullptr };

		const char* stateName{ "unknown" };

		VCD::PoseFlags poseFlags{};

		// Retry mechanism for player transformations.
		bool transitionRetryActive{ false };
		std::chrono::steady_clock::time_point transitionRetryUntil{};
		std::chrono::steady_clock::time_point nextTransitionRetry{};

		// Reapply saved preset after this delay on loading.
		bool postLoadApplyPending{ true };
		bool postLoadTimerStarted{ false };
		std::chrono::steady_clock::time_point postLoadApplyAt{};
	};

	struct PreviewState
	{
		bool active{ false };

		bool restorePending{ false };

		VCD::Preset preset{ VCD::Preset::kVanilla };

		std::chrono::steady_clock::time_point restoreAt{};
	};

	struct NPCPreviewState
	{
		bool active{ false };

		bool blockActorUpdates{ false };

		RE::ActorHandle actor{};

		VCD::Preset preset{ VCD::Preset::kVanilla };
	};

	struct NPCPresetState
	{
		RE::FormID formID{ 0 };

		RE::ActorHandle actor{};

		const RE::bhkCharacterController* controller{ nullptr };

		VCD::Preset preset{ VCD::Preset::kVanilla };

		const char* stateName{ "unknown" };

		VCD::PoseFlags poseFlags{};
	};

	struct NPCDynamicsState
	{
		std::vector<NPCPresetState> actors{};
		std::vector<RE::ActorHandle> poseFixedActors{};
		std::vector<RE::ActorHandle> nearbyActors{};
		std::chrono::steady_clock::time_point nextScan{};
	};

	struct DynamicsConfig
	{
#define PRESET_STATE2DEF(S, D) VCD::Preset S{ D };
		FOREACH_PRESET_STATE(PRESET_STATE2DEF)
	};

	inline PresetState& GetPresetState()
	{
		static PresetState state{};
		return state;
	}


	inline DynamicsConfig& GetConfig()
	{
		static DynamicsConfig config{};
		return config;
	}

	inline PreviewState& GetPreviewState()
	{
		static PreviewState state{};
		return state;
	}

	inline NPCPreviewState& GetNPCPreviewState()
	{
		static NPCPreviewState state{};
		return state;
	}

	inline NPCDynamicsState& GetNPCDynamicsState()
	{
		static NPCDynamicsState state{};
		return state;
	}

	inline bool HasActorHandleWithFormID(const std::vector<RE::ActorHandle>& a_handles, const RE::FormID& a_formID)
	{
		for (auto& handle : a_handles) {
			auto actorPtr = handle.get();
			auto* actor = actorPtr.get();
			if (actor && actor->GetFormID() == a_formID) {
				return true;
			}
		}

		return false;
	}

	inline VCD::Preset GetCellPreset(const RE::TESObjectCELL* a_cell)
	{
		auto& config = GetConfig();
		if (!a_cell) {
			return config.neutral;
		}

		return a_cell->IsInteriorCell() ? config.indoor : config.outdoor;
	}

	inline VCD::Preset GetCellCameraPreset(const RE::TESObjectCELL* a_cell)
	{
		auto& config = GetConfig();
		if (!a_cell) {
			return config.cameraNeutral;
		}

		return a_cell->IsInteriorCell() ? config.cameraIndoor : config.cameraOutdoor;
	}

	inline const char* GetCellStateName(const RE::TESObjectCELL* a_cell)
	{
		if (!a_cell) {
			return "unknown";
		}

		return a_cell->IsInteriorCell() ? "indoor" : "outdoor";
	}

	inline bool IsPresetCurrent(const VCD::Preset& a_preset)
	{
		const auto& state = GetPresetState();
		return state.applied && state.current == a_preset;
	}

	inline bool IsPresetPreviewed(const VCD::Preset& a_preset)
	{
		const auto& preview = GetPreviewState();
		return preview.active && preview.preset == a_preset;
	}

	inline bool IsWerewolf(const RE::Actor* a_actor)
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

	inline bool IsVampireLord(const RE::Actor* a_actor)
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

	inline bool IsSwimming(const RE::Actor* a_actor)
	{
		const auto* actorState = a_actor ? a_actor->AsActorState() : nullptr;
		return actorState && actorState->IsSwimming();
	}

	inline bool IsTransformationState(const char* a_state)
	{
		return std::strcmp(a_state, "werewolf") == 0 || std::strcmp(a_state, "vampireLord") == 0;
	}

	void ApplyCameraCollisionRadius(float a_radiusSkyrim);

	bool ApplyPreset(const RE::PlayerCharacter* a_player, const VCD::Preset& a_preset, const char* a_state, const bool& a_force = false);

	void ApplyCameraPreset(const VCD::Preset& a_preset);

	bool ApplyEnvironmentPreset(const RE::PlayerCharacter* a_player, const bool& a_force = false);

	bool CanApplyNPCDynamics(RE::Actor* a_actor, const RE::PlayerCharacter* a_player, const float& a_radiusSquared);

	VCD::Preset GetNPCPreset(const RE::Actor* a_actor, const char*& a_stateName);

	const VCD::CollisionData* GetNPCCollisionData(const RE::FormID& a_formID, const VCD::Preset& a_preset);

	void TrackPoseFixedNPC(RE::Actor* a_actor);

	bool ApplyNPCPreset(RE::Actor* a_actor, const VCD::Preset& a_preset, const char* a_stateName);

	bool StartPresetPreview(const RE::PlayerCharacter* a_player, const VCD::Preset& a_preset);

	bool StartNPCPresetPreview(RE::Actor* a_actor, const VCD::Preset& a_preset);

	void StopNPCPresetPreview();

	void SchedulePostLoadApply();

	void SchedulePreviewRestore(const float& a_delaySeconds);

	void RestoreNPCsToVanilla();

	void RestoreCameraToVanilla();

	void RestorePresetPreview(const RE::PlayerCharacter* a_player);

	void ClearRuntimeState();

	void RemapDeletedPreset(const VCD::Preset& a_preset);

	void Update(const RE::PlayerCharacter* a_player);

	void UpdateNPCs(const RE::PlayerCharacter* a_player);

}

#undef PRESET_STATE2DEF
