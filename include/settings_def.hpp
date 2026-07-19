#pragma once

#include "preset_states.hpp"

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

#define FOREACH_TOOL_BOOL_SETTING(S) \
	S(drawCollision, false) \
	S(drawNearbyActors, false) \
	S(drawCameraCollision, false) \
	S(autoDrawPreview, true)

#define FOREACH_DYNAMICS_BOOL_SETTING(S) \
	S(enableNPCDynamics, false) \
	S(enableCameraDynamics, false)

#define FOREACH_POSE_FIX_BOOL_SETTING(S) \
	S(fixPlayerSitting, true) \
	S(fixNPCSitting, true) \
	S(fixPlayerSneaking, true) \
	S(fixNPCSneaking, true)

#define FOREACH_BOOL_SETTING(S) \
	FOREACH_TOOL_BOOL_SETTING(S) \
	FOREACH_DYNAMICS_BOOL_SETTING(S) \
	FOREACH_POSE_FIX_BOOL_SETTING(S)

#define FOREACH_TOOL_FLOAT_SETTING(S) \
	S(drawLineThickness, 1.2F) \
	S(drawNPCLineThickness, 1.2F) \
	S(drawCameraLineThickness, 2.0F) \
	S(previewRestoreDelay, 3.0F)

#define FOREACH_DYNAMICS_FLOAT_SETTING(S) \
	S(nearbyActorScanRadius, 1500.0F) \
	S(nearbyActorScanInterval, 0.5F)

#define FOREACH_POSE_FIX_FLOAT_SETTING(S) \
	S(playerSittingScale, 0.65F) \
	S(npcSittingScale, 0.65F) \
	S(grindstoneSittingScale, 0.5F) \
	S(playerSneakingScale, 0.7F) \
	S(npcSneakingScale, 0.7F)

#define FOREACH_FLOAT_SETTING(S) \
	FOREACH_TOOL_FLOAT_SETTING(S) \
	FOREACH_DYNAMICS_FLOAT_SETTING(S) \
	FOREACH_POSE_FIX_FLOAT_SETTING(S)

#define FOREACH_TOOL_INT_SETTING(S) \
	S(logLevel, 2)

#define FOREACH_DYNAMICS_INT_SETTING(S) \
	S(nearbyActorScanLimit, 16)

#define FOREACH_INT_SETTING(S) \
	FOREACH_TOOL_INT_SETTING(S) \
	FOREACH_DYNAMICS_INT_SETTING(S)

#define FOREACH_COLOR_SETTING(S) \
	S(drawColor, 1.0F, 1.0F, 0.0F, 1.0F) \
	S(drawNPCColor, 1.0F, 0.0F, 1.0F, 1.0F) \
	S(drawCameraColor, 1.0F, 0.0F, 1.0F, 1.0F)

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
			bool relative{ false };
			VCD::CollisionData data{};
		};

		struct NPCActorPresetOverride
		{
			RE::FormID formID{ 0 };
			std::string name{};
			VCD::Preset preset{ VCD::Preset::kVanilla };
			VCD::CollisionData data{};
		};

		FOREACH_BOOL_SETTING(BOOL2DEF);
		FOREACH_FLOAT_SETTING(FLOAT2DEF);
		FOREACH_INT_SETTING(INT2DEF);
		FOREACH_PRESET_STATE(PRESET2DEF);
		FOREACH_COLOR_SETTING(COLOR2DEF);

		std::unordered_map<std::string, PresetOverride> presets{};
		std::unordered_map<std::string, PresetOverride> npcPresets{};
		std::unordered_map<std::string, PresetOverride> cameraPresets{};

		std::vector<NPCActorPresetOverride> npcActorPresets{};
	};

}
