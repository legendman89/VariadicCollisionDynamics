#pragma once

#include "menu.hpp"
#include "dynamics.hpp"
#include "settings.hpp"

namespace UI {

    inline constexpr auto kFixedLogComboWidth = 180.0F;
    inline constexpr auto kFixedDelaySliderWidth = 260.0F;

    inline constexpr const char* kLogLevels[] = {
        "Trace",
        "Debug",
        "Info",
        "Warning",
        "Error",
        "Critical",
        "Off"
    };

    void ClearDrawLines();

    void ApplyLogLevel(const int& a_level);

    bool InputStringSetting(const char* a_label, std::string& a_value);

    void RenderGlobalStatus(const std::string& a_label, std::string& a_editorID, const RE::TESGlobal* a_resolved, const char* a_inputID);

    void RenderCustomKey();

}
