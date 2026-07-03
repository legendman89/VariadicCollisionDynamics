#pragma once

namespace Hook {

    struct PlayerUpdate {

        static void thunk(RE::PlayerCharacter* player, float delta);

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

    struct MenuTopicManagerHook
    {
        static void Install();

        static RE::BSEventNotifyControl ProcessMenuOpenCloseEvent(
            RE::MenuTopicManager* a_this,
            const RE::MenuOpenCloseEvent* a_event,
            RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_eventSource);

        static inline REL::Relocation<decltype(ProcessMenuOpenCloseEvent)> func;
    };

    struct ThirdPersonState_SetRotation
    {
        static void thunk(
            RE::ThirdPersonState* a_state,
            float* rotation,
            bool a_flag,
            bool a_someFlag
        );

        static inline REL::Relocation<decltype(thunk)*> func;
        static void Install();
    };

    struct BhkSimpleShapePhantom_SetPosition
    {
        static void thunk(RE::bhkSimpleShapePhantom* phantom, RE::hkVector4* position);
        static inline REL::Relocation<decltype(thunk)*> func;
        static void Install();
    };

    void Install(); 

}
