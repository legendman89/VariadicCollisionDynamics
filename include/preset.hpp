#pragma once

#include "plugin.hpp"

#include <string_view>

namespace VCD {

    constexpr std::string_view kCharacterBumperNodeName = "CharacterBumper";

    enum class Preset
    {
        kVanillaLike,
        kPersonalSpace,
        kCompact,
        kBulky
    };

    struct PresetMesh
    {
        Preset preset{};
        std::string_view name{};
        std::string_view path{};

        RE::NiPointer<RE::NiNode> root{};
        RE::NiPointer<RE::bhkSPCollisionObject> spCollisionObject{};

        bool loaded{ false };
        bool foundCharacterBumper{ false };
        bool foundBhkSPCollisionObject{ false };

        RE::BSResource::ErrorCode loadResult{};
    };

    
    template <class Enum>
    constexpr auto ToUnderlying(const Enum& a_value) 
    {
        return static_cast<std::underlying_type_t<Enum>>(a_value);
    }

}