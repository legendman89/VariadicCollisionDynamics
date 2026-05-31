#pragma once

#include "plugin.hpp"
#include "logger.hpp"

#include <string_view>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace VCD {

    inline constexpr std::string_view kCharacterBumperNodeName = "CharacterBumper";
    inline constexpr std::string_view kTemplateMeshDir = R"(meshes\Variadic Collision Tweaks\Templates)";

    enum class Preset
    {
        kVanillaLike,
        kPersonalSpace,
        kCompact,
        kBulky
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
            float scale{ 1.0F };
        } bump;

        struct CapsuleInfo {

            float radius{ 0.0F };

            Vec3 point1{};
            Vec3 point2{};

            float height{ 0.0F };

        } capsule;

        void Log() {
            logger::info(
                " Collision Data:\n"
                "  Translation: X={}, Y={}, Z={}\n"
                "  Scale: {}\n"
                "  Radius: {}\n"
                "  Height: {}\n"
                "  First Point:  X={}, Y={}, Z={}\n"
                "  Second Point: X={}, Y={}, Z={}\n",
                bump.translation.x,
                bump.translation.y,
                bump.translation.z,
                bump.scale,
                capsule.radius,
                capsule.height,
                capsule.point1.x,
                capsule.point1.y,
                capsule.point1.z,
                capsule.point2.x,
                capsule.point2.y,
                capsule.point2.z);
        }
    };

    struct PresetMesh
    {
        Preset preset{};
        std::string name{};
        std::string path{};

        RE::NiPointer<RE::NiNode> root{};
        RE::NiPointer<RE::bhkSPCollisionObject> spCollisionObject{};

        CollisionData data;

        bool loaded{ false };
        bool foundCharacterBumper{ false };
        bool foundBhkSPCollisionObject{ false };
        bool foundCapsuleShape{ false };

        RE::BSResource::ErrorCode loadResult{};
    };

    
    template <class Enum>
    constexpr auto ToUnderlying(const Enum& a_value) 
    {
        return static_cast<std::underlying_type_t<Enum>>(a_value);
    }

    inline std::string MakeTemplatePath(const std::string& a_fileName)
    {
        std::string path{ kTemplateMeshDir };
        path += '\\';
        path += a_fileName;
        return path;
    }


}