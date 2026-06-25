#pragma once

namespace Hook {

    struct PlayerUpdate {

        static void thunk(RE::PlayerCharacter* player, float delta);

        static inline REL::Relocation<decltype(thunk)> func;

        static void Install();
    };

    struct SneakHandlerCanProcess {

        static bool thunk(RE::SneakHandler* a_this, RE::InputEvent* a_event);

        static inline REL::Relocation<decltype(thunk)> func;

        static void Install();
    };

    struct SneakHandlerProcessButton {

        static void thunk(
            RE::SneakHandler* a_this,
            RE::ButtonEvent* a_event,
            RE::PlayerControlsData* a_data);

        static inline REL::Relocation<decltype(thunk)> func;

        static void Install();
    };

}