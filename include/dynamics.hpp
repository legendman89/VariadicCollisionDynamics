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

	bool ApplyPreset(const RE::PlayerCharacter* a_player, const VCD::Preset& a_preset, const char* a_state, const bool& a_force = false);

	bool ApplyEnvironmentPreset(const RE::PlayerCharacter* a_player, const bool& a_force = false);

	bool StartPresetPreview(const RE::PlayerCharacter* a_player, const VCD::Preset& a_preset);

	void SchedulePreviewRestore(const float& a_delaySeconds);

	void RestorePresetPreview(const RE::PlayerCharacter* a_player);

	void Update(const RE::PlayerCharacter* a_player);

	struct PlayerCellEvent : RE::BSTEventSink<RE::BGSActorCellEvent>
	{
		static void RegisterEventSink();

		static PlayerCellEvent* GetSingleton()
		{
			static PlayerCellEvent singleton;
			return &singleton;
		}

	private:
		RE::BSEventNotifyControl ProcessEvent(
			const RE::BGSActorCellEvent* a_event,
			RE::BSTEventSource<RE::BGSActorCellEvent>*) override;
	};


	struct CombatEventSink : public RE::BSTEventSink<RE::TESCombatEvent>
	{
		CombatEventSink() = default;
		CombatEventSink(const CombatEventSink&) = delete;
		CombatEventSink(CombatEventSink&&) = delete;

		CombatEventSink& operator=(const CombatEventSink&) = delete;
		CombatEventSink& operator=(CombatEventSink&&) = delete;

		static void RegisterEventSink();

		static CombatEventSink* GetSingleton()
		{
			static CombatEventSink singleton;
			return &singleton;
		}

	private:
		RE::BSEventNotifyControl ProcessEvent(
			const RE::TESCombatEvent* event,
			RE::BSTEventSource<RE::TESCombatEvent>*) override;
	};


	void Install();


}
