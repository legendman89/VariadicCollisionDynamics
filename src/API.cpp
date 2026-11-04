#include "API.hpp"

namespace API {

    // User has defined a lock global variable to prevent then custom keybind from working
    //until a condition they define has been met
    bool CustomKeyGlobals::IsCustomKeyBindPresetLocked()
    {
        const auto* lock = CustomKeyGlobals::g_vLock;
        return lock && lock->value == 1.0F;
    }



}