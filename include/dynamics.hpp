#pragma once

#include "manager.hpp"

#include <chrono>

namespace Dynamics {

	struct PresetState
	{
		bool applied{ false };

		VCD::Preset current{ VCD::Preset::kVanillaLike };

		const RE::TESObjectCELL* lastCell{ nullptr };

		const char* stateName{ "unknown" };

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

	struct DynamicsConfig
	{
		VCD::Preset outdoor{ VCD::Preset::kPersonalSpace };
		VCD::Preset indoor{ VCD::Preset::kCompact };
		VCD::Preset combat{ VCD::Preset::kBulky };
		VCD::Preset werewolf{ VCD::Preset::kWerewolf };
		VCD::Preset vampireLord{ VCD::Preset::kVampireLord };
		VCD::Preset neutral{ VCD::Preset::kVanillaLike };
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

	VCD::Preset GetCellPreset(const RE::TESObjectCELL* a_cell);

	const char* GetCellStateName(const RE::TESObjectCELL* a_cell);

	bool IsPresetCurrent(const VCD::Preset& a_preset);

	bool IsPresetPreviewed(const VCD::Preset& a_preset);

	bool IsWerewolf(const RE::Actor* a_actor);

	bool IsVampireLord(const RE::Actor* a_actor);

	bool ApplyPreset(const RE::PlayerCharacter* a_player, const VCD::Preset& a_preset, const char* a_state, const bool& a_force = false);

	bool ApplyEnvironmentPreset(const RE::PlayerCharacter* a_player, const bool& a_force = false);

	bool StartPresetPreview(const RE::PlayerCharacter* a_player, const VCD::Preset& a_preset);

	void SchedulePreviewRestore(const float& a_delaySeconds);

	void RestorePresetPreview(const RE::PlayerCharacter* a_player);

	void Update(const RE::PlayerCharacter* a_player);

}
