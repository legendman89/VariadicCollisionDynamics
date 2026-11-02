#pragma once

namespace Hook {

    struct CharacterProxyProcessConstraints
    {
        static void thunk(
            RE::bhkCharProxyController* a_this,
            const RE::hkpCharacterProxy* a_proxy,
            const RE::hkArray<RE::hkpRootCdPoint>& a_manifold,
            RE::hkpSimplexSolverInput& a_input);

        static inline REL::Relocation<decltype(thunk)> func;

        static void Install();
    };

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


    struct CameraLinearCastHook
    {
        static void thunk(
            std::int64_t* param_1,   // unk120 phantom wrapper
            std::int64_t* param_2,   // bhkWorld
            float* param_3,   // TARGET pos - phantom moves here
            float* param_4,   // CURRENT pos - linear cast starts here  
            float* param_5,   // output
            std::uint64_t* param_6,   // hit actor
            float          param_7    // radius
        );

        static inline REL::Relocation<decltype(thunk)*> func;

        static void Install();
    };

  // Game processes collision filters through this hook, 
  // there is no way to tell through the arguemnts what is being psased through,
  // however by hooking different callers using "write_call" 
  // we can hook functions that call the func itself, that were used for specific game objects only
  //for example func1 is used only for actors / npcs, and func 2 is called only for the player

    struct CollisionFilterFunc
    {

        //npcs commented out because no way to know what NPC (needs to be disabled via menu for NPC Specific Presets)
      /*  static bool funcForActors(
            RE::bhkCollisionFilter* filter,
            RE::CFilter filterA,
            RE::CFilter filterB);
            */

        //player
        static bool funcForPlayer(
            RE::bhkCollisionFilter* filter,
            RE::CFilter filterA,
            RE::CFilter filterB);

      //  static inline REL::Relocation<decltype(funcForActors)> func1;
        static inline REL::Relocation<decltype(funcForPlayer)> originalFuncForPlayer;

        static void Install();
    };

    void Install(); 

}
