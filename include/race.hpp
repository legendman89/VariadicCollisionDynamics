#pragma once

#include "plugin.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

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

	inline CollisionLimitClass GetCollisionLimitClass(const RE::Actor* a_actor)
	{
		const auto race = GetSupportedNPCPresetRace(a_actor);
		if (race) {
			return SupportedNPCPresetLimitClass(*race);
		}

		if (a_actor && (a_actor->IsGuard() || const_cast<RE::Actor*>(a_actor)->HasKeywordString("ActorTypeNPC"))) {
			return CollisionLimitClass::kHumanoid;
		}

		return CollisionLimitClass::kDefault;
	}

	inline bool IsSupportedNPCPresetActor(RE::Actor* a_actor)
	{
		return a_actor && (a_actor->IsGuard() || a_actor->HasKeywordString("ActorTypeNPC") || GetSupportedNPCPresetRace(a_actor).has_value());
	}

}

#undef SUPPORTED_NPC_PRESET_RACE_ENUM
#undef SUPPORTED_NPC_PRESET_RACE_INFO
#undef FOREACH_SUPPORTED_NPC_PRESET_RACE
