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

   /* struct UpdateRigidBodyHelper
    {
        static void thunk(
            RE::bhkRigidBody* a_rigidBody,
            const RE::NiPoint3* a_pos,
            const float* a_rotData);

        static inline REL::Relocation<decltype(thunk)*> func;

        static void Install();
    };*/

   /* struct UpdatePhantomPositionHelper
    {
        static bool thunk(
            RE::PlayerCamera* a_camera,
            RE::NiPoint3* a_position,
            bool a_flag
        );

        static inline REL::Relocation<decltype(thunk)*> func;
        static void Install();
    };
    */

    struct ThirdPersonState_SetRotation
    {
        static void thunk(
            RE::ThirdPersonState* a_state,
            RE::NiPoint3* rotation,
            bool a_flag,
            bool a_someFlag
        );

        static inline REL::Relocation<decltype(thunk)*> func;
        static void Install();
    };

    void Install(); 

}
