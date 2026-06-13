#pragma once

#include "preset.hpp"

#include <array>

#define FOREACH_BOOL_SETTING(S) \
	S(drawCollision, false) \
	S(autoDrawPreview, true)

#define FOREACH_FLOAT_SETTING(S) \
	S(drawLineThickness, 1.2F) \
	S(previewRestoreDelay, 3.0F)

#define FOREACH_INT_SETTING(S) \
	S(logLevel, 1)

#define FOREACH_PRESET_SETTING(S) \
	S(outdoor, VCD::Preset::kPersonalSpace) \
	S(indoor, VCD::Preset::kCompact) \
	S(combat, VCD::Preset::kBulky) \
	S(neutral, VCD::Preset::kVanillaLike)

#define BOOL2DEF(S, D) bool S = D;
#define FLOAT2DEF(S, D) float S = D;
#define INT2DEF(S, D) int S = D;
#define PRESET2DEF(S, D) VCD::Preset S = D;

namespace Settings {

	struct VCDSettings
	{
		struct PresetOverride
		{
			bool edited{ false };
			VCD::CollisionData data{};
		};

		FOREACH_BOOL_SETTING(BOOL2DEF);
		FOREACH_FLOAT_SETTING(FLOAT2DEF);
		FOREACH_INT_SETTING(INT2DEF);
		FOREACH_PRESET_SETTING(PRESET2DEF);

		std::array<float, 4> drawColor{ 1.0F, 1.0F, 0.0F, 1.0F };
		std::array<PresetOverride, 4> presets{};
	};

}
