#pragma once

#include "plugin.hpp"
#include "logger.hpp"
#include "helper.hpp"

#include <string_view>
#include <filesystem>
#include <string>
#include <cmath>

namespace fs = std::filesystem;

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

namespace VCD {

    inline constexpr std::string_view kPresetDir = "presets";

    enum class Preset
    {
        kVanillaLike,
        kPersonalSpace,
        kCompact,
        kBulky
    };

    inline constexpr const char* PresetName(const Preset& a_preset)
    {
        switch (a_preset) {
        case Preset::kVanillaLike:
            return "Vanilla-like";
        case Preset::kPersonalSpace:
            return "Personal Space";
        case Preset::kCompact:
            return "Compact";
        case Preset::kBulky:
            return "Bulky";
        default:
            return "Unknown";
        }
    }

    inline constexpr const char* kPresetNames[] = {
        PresetName(Preset::kVanillaLike),
        PresetName(Preset::kPersonalSpace),
        PresetName(Preset::kCompact),
        PresetName(Preset::kBulky)
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
                " Collision Data:\n"
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
        std::string name{};
        std::string configPath{};

        CollisionData data;

        bool loaded{ false };
    };

    
}
