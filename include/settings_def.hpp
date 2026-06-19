#pragma once

#include "preset.hpp"

#include <array>
#include <string>
#include <vector>

#define FOREACH_TOOL_BOOL_SETTING(S) \
	S(drawCollision, false) \
	S(drawNearbyActors, false) \
	S(autoDrawPreview, true)

#define FOREACH_DYNAMICS_BOOL_SETTING(S) \
	S(enableNPCDynamics, false)

#define FOREACH_BOOL_SETTING(S) \
	FOREACH_TOOL_BOOL_SETTING(S) \
	FOREACH_DYNAMICS_BOOL_SETTING(S)

#define FOREACH_FLOAT_SETTING(S) \
	S(drawLineThickness, 1.2F) \
	S(drawNPCLineThickness, 1.2F) \
	S(previewRestoreDelay, 3.0F) \
	S(nearbyActorDrawRadius, 1500.0F) \
	S(nearbyActorScanInterval, 0.5F)

#define FOREACH_INT_SETTING(S) \
	S(logLevel, 1) \
	S(nearbyActorDrawLimit, 16)

#define FOREACH_COLOR_SETTING(S) \
	S(drawColor, 1.0F, 1.0F, 0.0F, 1.0F) \
	S(drawNPCColor, 1.0F, 0.0F, 1.0F, 1.0F)

#define FOREACH_PLAYER_PRESET_SETTING(S) \
	S(outdoor, VCD::Preset::kPersonalSpace) \
	S(indoor, VCD::Preset::kCompact) \
	S(combat, VCD::Preset::kBulky) \
	S(werewolf, VCD::Preset::kWerewolf) \
	S(vampireLord, VCD::Preset::kVampireLord) \
	S(neutral, VCD::Preset::kVanillaLike)

#define FOREACH_NPC_PRESET_SETTING(S) \
	S(npcNeutral, VCD::Preset::kVanillaLike) \
	S(npcCombat, VCD::Preset::kPersonalSpace) \
	S(guardNeutral, VCD::Preset::kCompact) \
	S(guardCombat, VCD::Preset::kBulky)

#define FOREACH_PRESET_SETTING(S) \
	FOREACH_PLAYER_PRESET_SETTING(S) \
	FOREACH_NPC_PRESET_SETTING(S)

#define BOOL2DEF(S, D) bool S = D;
#define FLOAT2DEF(S, D) float S = D;
#define INT2DEF(S, D) int S = D;
#define PRESET2DEF(S, D) VCD::Preset S = D;
#define COLOR2DEF(S, D0, D1, D2, D3) std::array<float, 4> S{ D0, D1, D2, D3 };

namespace Settings {

	struct VCDSettings
	{
		struct PresetOverride
		{
			bool edited{ false };
			VCD::CollisionData data{};
		};

		struct NPCActorPresetOverride
		{
			RE::FormID formID{ 0 };
			std::string name{};
			VCD::Preset preset{ VCD::Preset::kVanillaLike };
			VCD::CollisionData data{};
		};

		FOREACH_BOOL_SETTING(BOOL2DEF);
		FOREACH_FLOAT_SETTING(FLOAT2DEF);
		FOREACH_INT_SETTING(INT2DEF);
		FOREACH_PRESET_SETTING(PRESET2DEF);
		FOREACH_COLOR_SETTING(COLOR2DEF);

		std::array<PresetOverride, VCD::kPresetCount> presets{};
		std::array<PresetOverride, VCD::kPresetCount> npcPresets{};

		std::vector<NPCActorPresetOverride> npcActorPresets{};
	};

}
