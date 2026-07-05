#pragma once

#include "preset.hpp"

#define FOREACH_PLAYER_PRESET_STATE(S) \
	S(outdoor, VCD::Preset::kPersonalSpace) \
	S(indoor, VCD::Preset::kCompact) \
	S(combat, VCD::Preset::kBulky) \
	S(swimming, VCD::Preset::kSwimming) \
	S(werewolf, VCD::Preset::kWerewolf) \
	S(vampireLord, VCD::Preset::kVampireLord) \
	S(neutral, VCD::Preset::kVanilla)

#define FOREACH_NPC_PRESET_STATE(S) \
	S(npcNeutral, VCD::Preset::kNPCNeutral) \
	S(npcCombat, VCD::Preset::kNPCCombat) \
	S(guardNeutral, VCD::Preset::kGuardNeutral) \
	S(guardCombat, VCD::Preset::kGuardCombat)

#define FOREACH_CAMERA_PRESET_STATE(S) \
    S(cameraIndoor, VCD::Preset::kCameraIndoor) \
    S(cameraOutdoor, VCD::Preset::kCameraOutdoor) \
    S(cameraDialogue, VCD::Preset::kCameraDialogue) \
	S(cameraNeutral, VCD::Preset::kCameraVanilla)

#define FOREACH_PRESET_STATE(S) \
	FOREACH_PLAYER_PRESET_STATE(S) \
	FOREACH_NPC_PRESET_STATE(S) \
	FOREACH_CAMERA_PRESET_STATE(S)
