#pragma once

#include "manager.hpp"

#include <chrono>
#include <vector>

namespace Dynamics {

	struct PresetState
	{
		bool applied{ false };

		VCD::Preset current{ VCD::Preset::kVanillaLike };

		const RE::TESObjectCELL* lastCell{ nullptr };

		const char* stateName{ "unknown" };

		// Retry mechanism for player transformations.
		bool transitionRetryActive{ false };

		std::chrono::steady_clock::time_point transitionRetryUntil{};

		std::chrono::steady_clock::time_point nextTransitionRetry{};
	};

	struct PreviewState
	{
		bool active{ false };

		bool restorePending{ false };

		VCD::Preset preset{ VCD::Preset::kVanillaLike };

		std::chrono::steady_clock::time_point restoreAt{};
	};

	struct NPCPreviewState
	{
		bool active{ false };

		RE::ActorHandle actor{};

		VCD::Preset preset{ VCD::Preset::kVanillaLike };
	};

	struct NPCPresetState
	{
		RE::FormID formID{ 0 };

		RE::ActorHandle actor{};

		VCD::Preset preset{ VCD::Preset::kVanillaLike };

		const char* stateName{ "unknown" };
	};

	struct NPCDynamicsState
	{
		std::vector<NPCPresetState> actors{};
		std::vector<RE::ActorHandle> nearbyActors{};

		std::chrono::steady_clock::time_point nextScan{};
	};

	struct DynamicsConfig
	{
		VCD::Preset outdoor{ VCD::Preset::kPersonalSpace };
		VCD::Preset indoor{ VCD::Preset::kCompact };
		VCD::Preset combat{ VCD::Preset::kBulky };
		VCD::Preset werewolf{ VCD::Preset::kWerewolf };
		VCD::Preset vampireLord{ VCD::Preset::kVampireLord };
		VCD::Preset neutral{ VCD::Preset::kVanillaLike };
		VCD::Preset npcNeutral{ VCD::Preset::kVanillaLike };
		VCD::Preset npcCombat{ VCD::Preset::kPersonalSpace };
		VCD::Preset guardNeutral{ VCD::Preset::kCompact };
		VCD::Preset guardCombat{ VCD::Preset::kBulky };
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

	inline VCD::Preset GetCellPreset(const RE::TESObjectCELL* a_cell)
	{
		auto& config = GetConfig();
		if (!a_cell) {
			return config.neutral;
		}

		return a_cell->IsInteriorCell() ? config.indoor : config.outdoor;
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

	inline bool IsTransformationState(const char* a_state)
	{
		return std::strcmp(a_state, "werewolf") == 0 || std::strcmp(a_state, "vampireLord") == 0;
	}

	bool ApplyPreset(const RE::PlayerCharacter* a_player, const VCD::Preset& a_preset, const char* a_state, const bool& a_force = false);

	bool ApplyEnvironmentPreset(const RE::PlayerCharacter* a_player, const bool& a_force = false);

	bool StartPresetPreview(const RE::PlayerCharacter* a_player, const VCD::Preset& a_preset);

	bool StartNPCPresetPreview(RE::Actor* a_actor, const VCD::Preset& a_preset);

	void StopNPCPresetPreview();

	void RestoreNPCsToVanillaLike();

	void SchedulePreviewRestore(const float& a_delaySeconds);

	void RestorePresetPreview(const RE::PlayerCharacter* a_player);

	void Update(const RE::PlayerCharacter* a_player);

	void UpdateNPCs(const RE::PlayerCharacter* a_player);

}
