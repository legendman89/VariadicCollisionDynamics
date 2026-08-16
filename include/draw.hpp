#pragma once

#include "helper.hpp"
#include "settings.hpp"

#include <array>
#include <cmath>

namespace DebugAPI_IMPL::Draw {

    inline constexpr int32_t CAPSULE_SIDES = 16;

    inline bool IsSupported()
    {
        return !REL::Module::IsVR();
    }

    struct ActorCapsuleDrawContext
    {
        bool isPlayer{ false };
        RE::NiPoint3 actorPosition{};
        std::array<float, 4> color{};
        float lineThickness{ 1.0F };
    };

    inline void TransformActorCapsule(const RE::Actor* a_actor, const ActorCapsuleDrawContext& a_context, const RE::NiPoint3& a_vertexA, const RE::NiPoint3& a_vertexB, RE::NiPoint3& a_outA, RE::NiPoint3& a_outB)
    {
        const auto yaw = -a_actor->data.angle.z;
        const auto c = std::cos(yaw);
        const auto s = std::sin(yaw);
        a_outA = VCD::RotatePoint(a_vertexA, c, s) + a_context.actorPosition;
        a_outB = VCD::RotatePoint(a_vertexB, c, s) + a_context.actorPosition;
    }

    bool GetActorCapsuleDrawContext(const RE::Actor* a_actor, RE::bhkCharacterController* a_controller, ActorCapsuleDrawContext& a_context);

    void DrawPlayerBumper();

    void DrawNearbyActorBumpers();

    void DrawSphere(RE::bhkSimpleShapePhantom* a_phantom, const RE::NiColorA& a_color, float a_lineThickness);

    void DrawCameraBumper();

}
