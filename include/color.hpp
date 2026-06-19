#pragma once

#include "ui.hpp"

namespace UI::Color {

    inline constexpr GUI::ImVec4 kCTAOnBG{ 0.95F, 0.96F, 0.98F, 1.00F };
    inline constexpr GUI::ImVec4 kCTAOnHover{ 1.00F, 1.00F, 1.00F, 1.00F };
    inline constexpr GUI::ImVec4 kCTAOnActive{ 0.92F, 0.93F, 0.96F, 1.00F };
    inline constexpr GUI::ImVec4 kCTAOnText{ 0.10F, 0.12F, 0.16F, 1.00F };
    inline constexpr GUI::ImVec4 kCTAOffBG{ 0.35F, 0.37F, 0.40F, 0.55F };
    inline constexpr GUI::ImVec4 kCTAOffHover{ 0.40F, 0.42F, 0.46F, 0.60F };
    inline constexpr GUI::ImVec4 kCTAOffActive{ 0.32F, 0.34F, 0.38F, 0.55F };
    inline constexpr GUI::ImVec4 kCTAOffText{ 0.60F, 0.62F, 0.68F, 1.00F };
    inline constexpr GUI::ImVec4 kEditHover{ 0.28F, 0.38F, 0.50F, 0.85F };
    inline constexpr GUI::ImVec4 kEditActive{ 0.34F, 0.48F, 0.64F, 0.95F };
    inline constexpr GUI::ImVec4 kEditWindowBg{ 0.08F, 0.10F, 0.12F, 0.96F };
    inline constexpr GUI::ImVec4 kEditFrameBg{ 0.02F, 0.02F, 0.02F, 1.00F };
    inline constexpr GUI::ImVec4 kEditFrameHover{ 0.08F, 0.08F, 0.08F, 1.00F };
    inline constexpr GUI::ImVec4 kEditFrameActive{ 0.12F, 0.12F, 0.12F, 1.00F };

}
