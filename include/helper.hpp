#pragma once

#include "plugin.hpp"
#include "logger.hpp"

#include <array>
#include <cctype>
#include <cstring>
#include <string>
#include <filesystem>
#include <string_view>
#include <type_traits>

namespace VCD {

    namespace fs = std::filesystem;

    struct PoseFlags
    {
        bool isSitting{ false };
        bool isChildSittingOnKnees{ false };
        bool isGrindstone{ false };
        bool isSneaking{ false };

        bool operator==(const PoseFlags&) const = default;
    };

    inline std::string ToUTF8(const fs::path& a_path)
    {
        auto u8 = a_path.u8string();
        return std::string(reinterpret_cast<const char*>(u8.c_str()));
    }

    inline void ReplaceAll(std::string& s, const std::string& from, const std::string& to) {
        if (from.empty()) return;

        size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.length(), to);
            pos += to.length();
        }
    }

    template <class Enum>
    constexpr auto ToUnderlying(const Enum& a_value)
    {
        return static_cast<std::underlying_type_t<Enum>>(a_value);
    }

    inline fs::path GetGameRoot()
    {
        return fs::path(REL::Module::get().filename()).parent_path();
    }

     inline fs::path GetPluginsDir()
    {
        return GetGameRoot() / "Data" / "SKSE" / "Plugins";
    }

    inline fs::path GetPluginDataPath()
    {
        return GetPluginsDir() / PRODUCT_NAME;
    }

    inline bool IsDynamicCollisionAdjustmentInstalled()
    {
        static const bool installed = std::filesystem::exists(VCD::GetPluginsDir() / "DynamicCollisionAdjustment.dll");

        return installed;
    }

    inline float GetPresetScale()
    {
        return RE::bhkWorld::GetWorldScaleInverse();
    }

    inline RE::hkVector4 ToHkVector(const RE::NiPoint3& a_vec)
    {
        return RE::hkVector4(a_vec.x, a_vec.y, a_vec.z, 0.0f);
    }

    inline RE::NiPoint3 ToNiPoint3(const RE::hkVector4& a_vec)
    {
        return RE::NiPoint3(a_vec.quad.m128_f32[0], a_vec.quad.m128_f32[1], a_vec.quad.m128_f32[2]);
    }

    inline RE::NiColorA ToNiColorA(const std::array<float, 4>& a_color)
    {
        return RE::NiColorA(a_color[0], a_color[1], a_color[2], a_color[3]);
    }

    inline RE::NiPoint3 RotatePoint(const RE::NiPoint3& a_point, const float& a_cos, const float& a_sin)
    {
        return RE::NiPoint3(a_point.x * a_cos - a_point.y * a_sin, a_point.x * a_sin + a_point.y * a_cos, a_point.z);
    }

    inline char ToLowerASCII(const char& a_char)
    {
        return static_cast<char>(std::tolower(a_char));
    }

    template <class T>
    inline bool isSpace(const T& ch)
    {
        return (ch >= 9 && ch <= 13) || ch == 32;
    }

    inline bool isDigit(const char& ch)
    {
        return (ch ^ '0') <= 9;
    }

    inline void eatWS(char*& str)
    {
        while (isSpace(*str)) {
            ++str;
        }
    }

    inline std::string Trim(std::string_view a_value)
    {
        auto first = a_value.begin();
        auto last = a_value.end();

        while (first != last && isSpace(*first)) {
            ++first;
        }

        while (first != last && isSpace(*(last - 1))) {
            --last;
        }

        return std::string(first, last);
    }

    inline bool ContainsInsensitive(const std::string_view& a_text, const std::string_view& a_pattern)
    {
        if (a_pattern.empty() || a_pattern.size() > a_text.size()) {
            return false;
        }

        const auto first = ToLowerASCII(a_pattern.front());
        const auto maxIndex = a_text.size() - a_pattern.size();
        for (std::size_t i = 0; i <= maxIndex; ++i) {
            if (ToLowerASCII(a_text[i]) != first) {
                continue;
            }

            std::size_t j = 1;
            for (; j < a_pattern.size(); ++j) {
                if (ToLowerASCII(a_text[i + j]) != ToLowerASCII(a_pattern[j])) {
                    break;
                }
            }

            if (j == a_pattern.size()) {
                return true;
            }
        }

        return false;
    }

    inline std::string GetActorName(RE::Actor* a_actor)
    {
        if (!a_actor) {
            return "Actor";
        }

        const auto* name = a_actor->GetDisplayFullName();
        if (!name || std::strlen(name) == 0) {
            return "Actor";
        }

        return name;
    }

    inline const char* GetOccupiedFurnitureName(const RE::Actor* a_actor)
    {
        if (!a_actor) {
            return "";
        }

        const auto furnitureHandle = const_cast<RE::Actor*>(a_actor)->GetOccupiedFurniture();
        const auto furniturePtr = furnitureHandle.get();
        const auto* furniture = furniturePtr.get();
        const auto* name = furniture ? furniture->GetName() : nullptr;

        logger::trace("Found furniture {}", name ? name : "Unknown");

        return name ? name : "";
    }

}
