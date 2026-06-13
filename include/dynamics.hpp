#pragma once

#include "manager.hpp"

namespace Dynamics {

	struct PresetState
	{
		bool applied{ false };

		VCD::Preset current{ VCD::Preset::kVanillaLike };

		const RE::TESObjectCELL* lastCell{ nullptr };

		const char* stateName{ "unknown" };
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

	VCD::Preset GetCellPreset(const RE::TESObjectCELL* a_cell);

	const char* GetCellStateName(const RE::TESObjectCELL* a_cell);

	const char* PresetName(const VCD::Preset& a_preset);

	bool ApplyPreset(const RE::PlayerCharacter* a_player, const VCD::Preset& a_preset, const char* a_state);

	bool ApplyEnvironmentPreset(const RE::PlayerCharacter* a_player);

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
