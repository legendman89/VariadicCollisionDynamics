#pragma once

namespace Hook {

    struct PlayerUpdate {

        static void thunk(RE::PlayerCharacter* player, float delta);

        static inline REL::Relocation<decltype(thunk)> func;

        static void Install();
    };

    struct SneakHandlerCanProcess {

        static bool thunk(RE::InputEvent* a_event);

        static inline REL::Relocation<decltype(thunk)> func;

        static void Install();
    };

}