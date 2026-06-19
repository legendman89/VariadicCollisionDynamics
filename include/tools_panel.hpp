#pragma once

#include "menu.hpp"
#include "dynamics.hpp"
#include "settings.hpp"

namespace UI {

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

}
