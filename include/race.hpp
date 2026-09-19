#pragma once

#include "plugin.hpp"
#include "preset.hpp"
#include "manager.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string_view>
#include <string>
#include <vector>

#define FOREACH_SUPPORTED_NPC_PRESET_RACE(S) \
	S(Giant, "GiantRace", "Giant", CollisionLimitClass::kGiant) \
	S(C00GiantOutsideWhiterun, "C00GiantOutsideWhiterunRace", "Giant", CollisionLimitClass::kGiant) \
	S(Troll, "TrollRace", "Troll", CollisionLimitClass::kLargeCreature) \
	S(TrollFrost, "TrollFrostRace", "Troll", CollisionLimitClass::kLargeCreature) \
	S(Draugr, "DraugrRace", "Draugr", CollisionLimitClass::kHumanoid) \
	S(DraugrMagic, "DraugrMagicRace", "Draugr", CollisionLimitClass::kHumanoid) \
	S(Dremora, "DremoraRace", "Dremora", CollisionLimitClass::kHumanoid) \
	S(Falmer, "FalmerRace", "Falmer", CollisionLimitClass::kHumanoid) \
	S(Hagraven, "HagravenRace", "Hagraven", CollisionLimitClass::kLargeCreature) \
	S(Spriggan, "SprigganRace", "Spriggan", CollisionLimitClass::kLargeCreature) \
	S(SprigganMatron, "SprigganMatronRace", "Spriggan", CollisionLimitClass::kLargeCreature) \
	S(DragonPriest, "DragonPriestRace", "DragonPriest", CollisionLimitClass::kHumanoid) \
	S(Skeleton, "skeletonRace", "Skeleton", CollisionLimitClass::kHumanoid) \
	S(RigidSkeleton, "RigidSkeletonRace", "Skeleton", CollisionLimitClass::kHumanoid) \
	S(SkeletonNecro, "SkeletonNecroRace", "Skeleton", CollisionLimitClass::kHumanoid) \
	S(SkeletonNecroPriest, "SkeletonNecroPriestRace", "Skeleton", CollisionLimitClass::kHumanoid) \
	S(AtronachFlame, "AtronachFlameRace", "FlameAtronach", CollisionLimitClass::kLargeCreature) \
	S(AtronachFrost, "AtronachFrostRace", "FrostAtronach", CollisionLimitClass::kLargeCreature) \
	S(AtronachStorm, "AtronachStormRace", "StormAtronach", CollisionLimitClass::kLargeCreature)

namespace VCD::Race {

	enum class SupportedNPCPresetRace
	{
#define SUPPORTED_NPC_PRESET_RACE_ENUM(S, E, P, L) k##S,
		FOREACH_SUPPORTED_NPC_PRESET_RACE(SUPPORTED_NPC_PRESET_RACE_ENUM)
		kTotal
	};

	enum class CollisionLimitClass
	{
		kPlayer,
		kHumanoid,
		kGiant,
		kLargeCreature,
		kDefault,
		kCamera,
		kTotal
	};

	struct SupportedNPCPresetRaceInfo
	{
		SupportedNPCPresetRace race{};
		std::string_view editorID{};
		std::string_view presetName{};
		CollisionLimitClass limitClass{ CollisionLimitClass::kDefault };
	};

	struct RegisteredRace
	{
		std::string plugin{}, editorID{}, presetKey{};
		RE::TESRace* race{ nullptr };
		RE::FormID localFormID{ 0 };
		CollisionLimitClass limitClass{ CollisionLimitClass::kDefault };
	};

	struct RaceRegistry
	{
		std::vector<RegisteredRace> races{};
		bool canSave{ false };
	};

	struct RegisteredRaceLimit
	{
		const char* key;
		const char* label;
		CollisionLimitClass limitClass;
	};

	void LoadRegisteredRaces();

	bool RegisterRace(RE::TESRace* a_race, Preset a_preset, CollisionLimitClass a_limitClass);

	const RegisteredRace* FindRegisteredRace(const RE::Actor* a_actor);

	inline RaceRegistry& GetRaceRegistry()
	{
		static RaceRegistry registry;
		return registry;
	}

	inline constexpr std::array kRegisteredRaceLimits{
		RegisteredRaceLimit{ "Default", "Dynamics.NPC.Limits.Default", CollisionLimitClass::kDefault },
		RegisteredRaceLimit{ "Humanoid", "Dynamics.NPC.Limits.Humanoid", CollisionLimitClass::kHumanoid },
		RegisteredRaceLimit{ "Giant", "Dynamics.NPC.Limits.Giant", CollisionLimitClass::kGiant },
		RegisteredRaceLimit{ "LargeCreature", "Dynamics.NPC.Limits.LargeCreature", CollisionLimitClass::kLargeCreature }
	};

	template <class Value, class Member>
	inline const RegisteredRaceLimit* FindRegisteredRaceLimit(const Value& a_value, Member RegisteredRaceLimit::* a_member)
	{
		for (const auto& option : kRegisteredRaceLimits) {
			if (option.*a_member == a_value) {
				return &option;
			}
		}
		return nullptr;
	}

	inline Preset GetRegisteredRacePreset(const RegisteredRace& a_registration)
    {
        const auto& manager = Manager::GetSingleton();
        const auto* preset = manager.GetPresetConfig(a_registration.presetKey);
        return preset && !manager.IsCameraPreset(preset->preset) ? preset->preset : Preset::kVanilla;
    }

	inline std::filesystem::path GetRegisteredRacesPath()
    {
        return GetPluginDataPath() / "RegisteredRaces.json";
    }

#define SUPPORTED_NPC_PRESET_RACE_INFO(S, E, P, L) SupportedNPCPresetRaceInfo{ SupportedNPCPresetRace::k##S, E, P, L },
	inline constexpr std::array<SupportedNPCPresetRaceInfo, static_cast<size_t>(SupportedNPCPresetRace::kTotal)> kSupportedNPCPresetRaces
	{
		FOREACH_SUPPORTED_NPC_PRESET_RACE(SUPPORTED_NPC_PRESET_RACE_INFO)
	};

	inline std::optional<SupportedNPCPresetRace> SupportedNPCPresetRaceFromEditorID(std::string_view a_editorID)
	{
		for (const auto& raceInfo : kSupportedNPCPresetRaces) {
			if (raceInfo.editorID == a_editorID) {
				return raceInfo.race;
			}
		}

		return std::nullopt;
	}

	inline std::string_view SupportedNPCPresetName(const SupportedNPCPresetRace& a_race)
	{
		const auto index = static_cast<size_t>(a_race);
		return index < kSupportedNPCPresetRaces.size() ? kSupportedNPCPresetRaces[index].presetName : std::string_view{};
	}

	inline CollisionLimitClass SupportedNPCPresetLimitClass(const SupportedNPCPresetRace& a_race)
	{
		const auto index = static_cast<size_t>(a_race);
		return index < kSupportedNPCPresetRaces.size() ? kSupportedNPCPresetRaces[index].limitClass : CollisionLimitClass::kDefault;
	}

	inline std::optional<SupportedNPCPresetRace> GetSupportedNPCPresetRace(const RE::Actor* a_actor)
	{
		if (!a_actor) {
			return std::nullopt;
		}

		const auto* race = a_actor->GetRace();
		if (!race) {
			return std::nullopt;
		}

		const auto* editorID = race->GetFormEditorID();
		return editorID ? SupportedNPCPresetRaceFromEditorID(editorID) : std::nullopt;
	}

	inline std::string_view GetSupportedNPCPresetName(const RE::Actor* a_actor)
	{
		const auto race = GetSupportedNPCPresetRace(a_actor);
		return race ? SupportedNPCPresetName(*race) : std::string_view{};
	}

	inline CollisionLimitClass GetCollisionLimitClass(const RE::Actor* a_actor, const RegisteredRace* a_registration)
	{
		if (a_registration) {
			return a_registration->limitClass;
		}
		const auto race = GetSupportedNPCPresetRace(a_actor);
		if (race) {
			return SupportedNPCPresetLimitClass(*race);
		}

		if (a_actor && (a_actor->IsGuard() || const_cast<RE::Actor*>(a_actor)->HasKeywordString("ActorTypeNPC"))) {
			return CollisionLimitClass::kHumanoid;
		}

		return CollisionLimitClass::kDefault;
	}

	inline CollisionLimitClass GetCollisionLimitClass(const RE::Actor* a_actor)
	{
		return GetCollisionLimitClass(a_actor, FindRegisteredRace(a_actor));
	}

	inline bool IsSupportedNPCPresetActor(RE::Actor* a_actor, const RegisteredRace* a_registration)
	{
		return a_actor && (a_registration || a_actor->IsGuard() || a_actor->HasKeywordString("ActorTypeNPC") || GetSupportedNPCPresetRace(a_actor).has_value());
	}

	inline bool IsSupportedNPCPresetActor(RE::Actor* a_actor)
	{
		return IsSupportedNPCPresetActor(a_actor, FindRegisteredRace(a_actor));
	}

}

#undef SUPPORTED_NPC_PRESET_RACE_ENUM
#undef SUPPORTED_NPC_PRESET_RACE_INFO
#undef FOREACH_SUPPORTED_NPC_PRESET_RACE
