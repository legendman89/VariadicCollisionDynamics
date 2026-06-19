#pragma once

#include "plugin.hpp"

#include <array>
#include <filesystem>
#include <string>
#include <type_traits>

namespace VCD {

    namespace fs = std::filesystem;

    inline std::string ToUTF8(const fs::path& a_path)
    {
        auto u8 = a_path.u8string();
        return std::string(reinterpret_cast<const char*>(u8.c_str()));
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

    inline fs::path GetPluginDataPath()
    {
        return GetGameRoot() / "Data" / "SKSE" / "Plugins" / PRODUCT_NAME;
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

}
