#include "API.hpp"
#include "dynamics.hpp"

namespace API {

    void CustomKeyGlobals::SyncStateGlobal()
    {
        auto* state = API::CustomKeyGlobals::g_vState;
        if (!state) {
            return;
        }
        state->value = Dynamics::GetCustomKeybindHeld().load() ? 1.0F : 0.0F;
    }

    // User has defined a lock global variable to prevent then custom keybind from working
    //until a condition they define has been met
    bool CustomKeyGlobals::IsCustomKeyBindPresetLocked()
    {
        const auto* lock = CustomKeyGlobals::g_vLock;
        return lock && lock->value == 1.0F;
    }

}