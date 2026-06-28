#pragma once

#include "plugin.hpp"
#include "logger.hpp"
#include "helper.hpp"

#include <string_view>
#include <string>
#include <array>
#include <vector>
#include <cmath>
#include <cstddef>
#include <limits>
#include <type_traits>

#define FOREACH_COLLISION_DATA_FIELD(S) \
    S(bump.translation.x) \
    S(bump.translation.y) \
    S(bump.translation.z) \
    S(capsule.radius) \
    S(capsule.height) \
    S(capsule.point1.x) \
    S(capsule.point1.y) \
    S(capsule.point1.z) \
    S(capsule.point2.x) \
    S(capsule.point2.y) \
    S(capsule.point2.z)

#define FOREACH_PRESET(S) \
    S(Vanilla, "Vanilla") \
    S(PersonalSpace, "Personal Space") \
    S(Compact, "Compact") \
    S(Bulky, "Bulky") \
    S(Werewolf, "Werewolf") \
    S(VampireLord, "Vampire Lord") \
    S(NPCNeutral, "NPC Neutral") \
    S(NPCCombat, "NPC Combat") \
    S(GuardNeutral, "Guard Neutral") \
    S(GuardCombat, "Guard Combat") \
    S(Giant, "Giant") \
    S(Troll, "Troll") \
    S(Draugr, "Draugr")

namespace VCD {

    enum class Preset
    {
#define PRESET_ENUM(Name, DisplayName) k##Name,
        FOREACH_PRESET(PRESET_ENUM)
        kTotal
    };

    inline constexpr std::string_view kPresetDir = "presets";
    inline constexpr size_t kBuiltInPresetCount = static_cast<size_t>(Preset::kTotal);
    inline constexpr Preset kInvalidPreset = static_cast<Preset>(std::numeric_limits<std::underlying_type_t<Preset>>::max());

#define PRESET_KEY_ENTRY(Name, DisplayName) #Name,
    inline constexpr std::array<const char*, kBuiltInPresetCount> kPresetKeys = {
        FOREACH_PRESET(PRESET_KEY_ENTRY)
    };

#define PRESET_NAME_ENTRY(Name, DisplayName) DisplayName,
    inline constexpr std::array<const char*, kBuiltInPresetCount> kPresetNames = {
        FOREACH_PRESET(PRESET_NAME_ENTRY)
    };

    struct Vec3
    {
        float x{};
        float y{};
        float z{};

        void Set(const RE::hkVector4& a_vec) {
            alignas(16) float values[4]{};
            _mm_store_ps(values, a_vec.quad);
            x = values[0], y = values[1], z = values[2];
        }
    };


    struct CollisionData
    {
        struct BumperNodeInfo
        {
            RE::NiPoint3 translation{};
        } bump;

        struct CapsuleInfo {

            float radius{ 0.0F };

            Vec3 point1{};
            Vec3 point2{};

            float height{ 0.0F };

        } capsule;

        void Log() {

#define COLLISION_FIELD2LOG(F) ,F

            logger::info(
                "\n"
                "  Translation: X={}, Y={}, Z={}\n"
                "  Radius: {}\n"
                "  Height: {}\n"
                "  First Point:  X={}, Y={}, Z={}\n"
                "  Second Point: X={}, Y={}, Z={}\n"
                FOREACH_COLLISION_DATA_FIELD(COLLISION_FIELD2LOG));
        }

        void RecalculateHeight()
        {
            const auto x = capsule.point1.x - capsule.point2.x;
            const auto y = capsule.point1.y - capsule.point2.y;
            const auto z = capsule.point1.z - capsule.point2.z;
            capsule.height = std::sqrt((x * x) + (y * y) + (z * z));
        }

        bool IsSame(const CollisionData& a_other) const
        {
#define COLLISION_FIELD2EQ(F) F == a_other.F &&

            return FOREACH_COLLISION_DATA_FIELD(COLLISION_FIELD2EQ) true;
        }
    };

    struct PresetConfig
    {
        Preset preset{};
        std::string key{};
        std::string name{};

        CollisionData data;

        bool loaded{ false };
        bool builtIn{ false };
    };

    inline void InitializePresetConfigs(std::vector<PresetConfig>& a_presets)
    {
        a_presets.clear();
        a_presets.reserve(kBuiltInPresetCount);
        for (size_t i = 0; i < kBuiltInPresetCount; ++i) {
            a_presets.push_back({
                static_cast<Preset>(i),
                kPresetKeys[i],
                kPresetNames[i],
                {},
                false,
                true
            });
        }
    }

    inline void RemapPresetAfterDeletion(Preset& a_value, const Preset& a_deleted, const Preset& a_replacement = Preset::kVanilla)
    {
        const auto value = ToUnderlying(a_value);
        const auto deleted = ToUnderlying(a_deleted);
        if (value == deleted) {
            a_value = a_replacement;
        }
        else if (value > deleted) {
            a_value = static_cast<Preset>(value - 1);
        }
    }

    
}

#undef PRESET_ENUM
#undef PRESET_KEY_ENTRY
#undef PRESET_NAME_ENTRY
#undef COLLISION_FIELD2LOG
#undef COLLISION_FIELD2EQ
