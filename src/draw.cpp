#include "draw.hpp"

#include "dynamics.hpp"
#include "helper.hpp"
#include "manager.hpp"
#include "scan.hpp"

#include <CLibUtilsQTR/DrawDebug.hpp>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <vector>

namespace DebugAPI_IMPL::Draw {

    void DrawCircleAxis(const RE::NiPoint3& center, const RE::NiPoint3& xAxis, const RE::NiPoint3& yAxis,
        float radius, uint32_t segments, int liftetimeMS = 10,
        const RE::NiColorA& color = { 1.0f, 0.0f, 0.0f, 1.0f }, float lineThickness = 1)
    {
        const float angleDelta = RE::NI_TWO_PI / segments;
        RE::NiPoint3 lastVertex = center + xAxis * radius;

        for (uint32_t i = 0; i < segments; i++) {
            RE::NiPoint3 vertex = center + (xAxis * cosf(angleDelta * (i + 1)) + yAxis * sinf(angleDelta * (i + 1))) * radius;
            DebugAPI::GetSingleton()->DrawLineForMS(lastVertex, vertex, liftetimeMS, color, lineThickness);
            lastVertex = vertex;
        }
    }

    void DrawHalfCircle(const RE::NiPoint3& center, const RE::NiPoint3& xAxis, const RE::NiPoint3& yAxis,
        float radius, uint32_t segments, int liftetimeMS = 10,
        const RE::NiColorA& color = { 1.0f, 0.0f, 0.0f, 1.0f }, float lineThickness = 1)
    {
        const float angleDelta = RE::NI_TWO_PI / segments;
        RE::NiPoint3 lastVertex = center + xAxis * radius;

        for (uint32_t i = 0; i < segments / 2; i++) {
            RE::NiPoint3 vertex = center + (xAxis * cosf(angleDelta * (i + 1)) + yAxis * sinf(angleDelta * (i + 1))) * radius;
            DebugAPI::GetSingleton()->DrawLineForMS(lastVertex, vertex, liftetimeMS, color, lineThickness);
            lastVertex = vertex;
        }
    }

    void DrawCollapsedCapsule(const RE::NiPoint3& center, float radius, int liftetimeMS = 10,
        const RE::NiColorA& color = { 1.0f, 0.0f, 0.0f, 1.0f }, float lineThickness = 1)
    {
        const auto xAxis = RE::NiPoint3(1.0F, 0.0F, 0.0F);
        const auto yAxis = RE::NiPoint3(0.0F, 1.0F, 0.0F);
        const auto zAxis = RE::NiPoint3(0.0F, 0.0F, 1.0F);

        DrawCircleAxis(center, xAxis, yAxis, radius, CAPSULE_SIDES, liftetimeMS, color, lineThickness);
        DrawCircleAxis(center, xAxis, zAxis, radius, CAPSULE_SIDES, liftetimeMS, color, lineThickness);
        DrawCircleAxis(center, yAxis, zAxis, radius, CAPSULE_SIDES, liftetimeMS, color, lineThickness);
    }

    // Based on TrueHUD's capsule drawing method but uses DebugAPI's line drawing.
    // I Optimized it to handle collapsing capsules. Later I might replace it entirely
    // with high-quality version similar to nifskope's.
    void DrawCapsule(const RE::NiPoint3& vertexA, const RE::NiPoint3& vertexB, float radius,
        int liftetimeMS = 10, const RE::NiColorA& color = { 1.0f, 0.0f, 0.0f, 1.0f },
        float lineThickness = 1)
    {
        RE::NiPoint3 zAxis = vertexA - vertexB;
        const auto axisLengthSquared = (zAxis.x * zAxis.x) + (zAxis.y * zAxis.y) + (zAxis.z * zAxis.z);
        if (axisLengthSquared <= 1.0e-4F) {
            DrawCollapsedCapsule((vertexA + vertexB) * 0.5F, radius, liftetimeMS, color, lineThickness);
            return;
        }

        zAxis.Unitize();

        RE::NiPoint3 upVector = (fabs(zAxis.z) < (1.0f - 1.e-4f)) ? RE::NiPoint3(0.0f, 0.0f, 1.0f) : RE::NiPoint3(1.0f, 0.0f, 0.0f);
        RE::NiPoint3 xAxis = upVector.UnitCross(zAxis);
        RE::NiPoint3 yAxis = zAxis.Cross(xAxis);

        DrawCircleAxis(vertexA, xAxis, yAxis, radius, CAPSULE_SIDES, liftetimeMS, color, lineThickness);
        DrawCircleAxis(vertexB, xAxis, yAxis, radius, CAPSULE_SIDES, liftetimeMS, color, lineThickness);

        DrawHalfCircle(vertexA, yAxis, zAxis, radius, CAPSULE_SIDES, liftetimeMS, color, lineThickness);
        DrawHalfCircle(vertexA, xAxis, zAxis, radius, CAPSULE_SIDES, liftetimeMS, color, lineThickness);

        RE::NiPoint3 negZAxis = -zAxis;
        DrawHalfCircle(vertexB, yAxis, negZAxis, radius, CAPSULE_SIDES, liftetimeMS, color, lineThickness);
        DrawHalfCircle(vertexB, xAxis, negZAxis, radius, CAPSULE_SIDES, liftetimeMS, color, lineThickness);

        DebugAPI::GetSingleton()->DrawLineForMS(vertexA + xAxis * radius, vertexB + xAxis * radius, liftetimeMS, color, lineThickness);
        DebugAPI::GetSingleton()->DrawLineForMS(vertexA - xAxis * radius, vertexB - xAxis * radius, liftetimeMS, color, lineThickness);
        DebugAPI::GetSingleton()->DrawLineForMS(vertexA + yAxis * radius, vertexB + yAxis * radius, liftetimeMS, color, lineThickness);
        DebugAPI::GetSingleton()->DrawLineForMS(vertexA - yAxis * radius, vertexB - yAxis * radius, liftetimeMS, color, lineThickness);
    }

    bool DrawActorBumper(const RE::Actor* a_actor)
    {
        if (!a_actor) {
            return false;
        }

        const auto* player = RE::PlayerCharacter::GetSingleton();
		const bool isPlayer = a_actor == player;

        VCD::Manager::ActorBumperContext context{};
        if (!VCD::Manager::GetSingleton().GetActorBumperContext(a_actor, context, isPlayer)) {
            return false;
        }

        RE::BSReadLockGuard lock(context.world->worldLock);

        auto* bumper = VCD::Manager::GetSingleton().FindWorldCharacterBumperShape(context.controller);
        if (!bumper) {
            return false;
        }

        ActorCapsuleDrawContext drawContext{};
        if (!GetActorCapsuleDrawContext(a_actor, context.controller, drawContext)) {
            return false;
        }

        RE::NiPoint3 vertexA = VCD::ToNiPoint3(bumper->vertexA) * VCD::GetPresetScale();
        RE::NiPoint3 vertexB = VCD::ToNiPoint3(bumper->vertexB) * VCD::GetPresetScale();
        float radius = bumper->radius * VCD::GetPresetScale();
        RE::NiPoint3 a{};
        RE::NiPoint3 b{};
        TransformActorCapsule(a_actor, drawContext, vertexA, vertexB, a, b);

        DrawCapsule(a, b, radius, 1, VCD::ToNiColorA(drawContext.color), drawContext.lineThickness);
        return true;
    }

    void DrawSphere(RE::bhkSimpleShapePhantom* a_phantom, const RE::NiColorA& a_color, float a_lineThickness)
    {
        auto* playerCamera = RE::PlayerCamera::GetSingleton();
        if (!playerCamera) return;

        auto& rt = playerCamera->GetRuntimeData2();
        float yaw = rt.yaw;
        RE::NiPoint3 forward{ std::cos(yaw), std::sin(yaw), 0.0f };

        auto& manager = VCD::Manager::GetSingleton();
        auto* sphere = manager.GetCameraSphereShape(a_phantom);
        if (!sphere) return;

        auto* camHkpPhantom = manager.GetCameraSimpleShapePhantom(a_phantom);
        if (!camHkpPhantom) return;

        auto& translation = camHkpPhantom->motionState.transform.translation;
        RE::NiPoint3 hkPos(
            translation.quad.m128_f32[0],
            translation.quad.m128_f32[1],
            translation.quad.m128_f32[2]
        );

        logger::info(
            "HK POS RAW: x={} y={} z={}",
            translation.quad.m128_f32[0],
            translation.quad.m128_f32[1],
            translation.quad.m128_f32[2]
        );

        RE::NiPoint3 realCollisionPos = hkPos * VCD::GetPresetScale();

        // push in front so it's visible, but offset relative to the real collision center
        RE::NiPoint3 projectedPos = realCollisionPos + forward * 100.0f;

        const auto radius = sphere->radius * VCD::GetPresetScale();

        DebugAPI_IMPL::DebugAPI::GetSingleton()->DrawSphere(projectedPos, radius, 10, a_color, a_lineThickness);
    }

    void DrawCameraBumper()
    {
        auto* playerCamera = RE::PlayerCamera::GetSingleton();
        if (!playerCamera) return;

        auto& cameraRTD = playerCamera->GetRuntimeData();
        if (!cameraRTD.unk120) return;

        struct CameraCollisionPhantoms
        {
            RE::NiPointer<RE::bhkSimpleShapePhantom> unk00;
            //I can't find a instance of where 08 is used probobly special camrera state
            RE::NiPointer<RE::bhkSimpleShapePhantom> unk08;
        };

        auto* cameraPhantoms = reinterpret_cast<CameraCollisionPhantoms*>(cameraRTD.unk120);
        const auto& settings = Settings::GetSettings();
        const auto color = VCD::ToNiColorA(settings.drawCameraColor);

        DrawSphere(cameraPhantoms->unk00.get(), color, settings.drawCameraLineThickness);
    }

    void DrawPlayerBumper()
    {
        auto* player = RE::PlayerCharacter::GetSingleton();
        DrawActorBumper(player);
    }

    void DrawNearbyActorBumpers()
    {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return;
        }

        auto& state = Scan::GetNearbyActorScanState();
        const auto& settings = Settings::GetSettings();
        const auto now = std::chrono::steady_clock::now();
        bool scanned = false;
        if (now >= state.nextScan) {
            Scan::RefreshNearbyActorCache(player);
            scanned = true;
            const auto interval = std::chrono::duration<float>(settings.nearbyActorScanInterval);
            state.nextScan = now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(interval);
        }

        int drawnCount = 0;
        int filteredCount = 0;
        const auto radiusSquared = settings.nearbyActorScanRadius * settings.nearbyActorScanRadius;
        for (auto& handle : state.handles) {
            auto actorPtr = handle.get();
            auto* actor = actorPtr.get();
            if (Scan::CanScanNearbyActor(actor, player, radiusSquared)) {
                if (DrawActorBumper(actor)) {
                    drawnCount++;
                }
            } else {
                filteredCount++;
            }
        }

        if (scanned) {
            logger::debug("Nearby actor draw: cached={}, drawn={}, filtered={}", state.handles.size(), drawnCount, filteredCount);
        }
    }

    bool GetActorCapsuleDrawContext(const RE::Actor* a_actor, RE::bhkCharacterController* a_controller, ActorCapsuleDrawContext& a_context)
    {
        if (!a_actor || !a_controller) {
            return false;
        }

        const auto* player = RE::PlayerCharacter::GetSingleton();
        a_context.isPlayer = a_actor == player;

        RE::hkVector4 controllerPositionHK;
        a_controller->GetPosition(controllerPositionHK, false);
        const auto controllerPosition = VCD::ToNiPoint3(controllerPositionHK) * VCD::GetPresetScale();
        a_context.actorPosition = a_actor->GetPosition();
        if (a_context.isPlayer) {
            a_context.actorPosition = controllerPosition;
        }
        else {
            a_context.actorPosition.z = controllerPosition.z;
        }

        const auto& settings = Settings::GetSettings();
        a_context.color = a_context.isPlayer ? settings.drawColor : settings.drawNPCColor;
        a_context.lineThickness = a_context.isPlayer ? settings.drawLineThickness : settings.drawNPCLineThickness;
        return true;
    }

}
