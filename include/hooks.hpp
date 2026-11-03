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

    struct CollisionFilterHooks
    {
        static void Install();

        static std::uint8_t* actorFilter(
            const RE::hkpShapeCollectionFilter* a_this,
            std::uint8_t* a_result,
            const RE::hkpCollisionInput& a_input,
            const RE::hkpCdBody& a_bodyA,
            const RE::hkpCdBody& a_bodyB,
            const RE::hkpShapeContainer& a_shapeA,
            const RE::hkpShapeContainer& a_shapeB,
            RE::hkpShapeKey a_keyA,
            RE::hkpShapeKey a_keyB);

        static bool playerFilter(
            RE::bhkCollisionFilter* filter,
            RE::CFilter filterA,
            RE::CFilter filterB);

        static inline REL::Relocation<decltype(playerFilter)> playerFunc;

        static inline REL::Relocation<decltype(actorFilter)*> actorFunc;
    };

    struct MeleeHitHookSE
    {
        static void func(
            RE::Character* a_aggressor,
            RE::Actor* a_victim,
            std::int64_t a3,
            char a4,
            float a5);

        static inline REL::Relocation<decltype(func)> originalFunc;

        static void Install();
    };

    struct MeleeHitHookAE
    {
        using HitFunction = void(
            RE::Actor*,
            RE::Actor*,
            std::int64_t,
            char
            );

        static void func(
            RE::Actor* a_aggressor,
            RE::Actor* a_victim,
            std::int64_t a3,
            char a4
        );

        static inline HitFunction* originalFunc = nullptr;

        static void Install();
    };

  

    void Install(); 

}
