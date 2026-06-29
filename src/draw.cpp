#include "draw.hpp"

#include "helper.hpp"
#include "manager.hpp"

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

    // Based on TrueHUD's capsule drawing method but uses DebugAPI's line drawing.
    void DrawCapsule(const RE::NiPoint3& vertexA, const RE::NiPoint3& vertexB, float radius,
        int liftetimeMS = 10, const RE::NiColorA& color = { 1.0f, 0.0f, 0.0f, 1.0f },
        float lineThickness = 1)
    {
        RE::NiPoint3 zAxis = vertexA - vertexB;
        zAxis.Unitize();

        RE::NiPoint3 upVector = (fabs(zAxis.z) < (1.0f - 1.e-4f)) ? RE::NiPoint3(0.0f, 0.0f, 1.0f)
            : RE::NiPoint3(1.0f, 0.0f, 0.0f);
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

    bool CanDrawActorBumper(const RE::Actor* a_actor, const RE::PlayerCharacter* a_player, const float& a_radiusSquared, NearbyActorDrawState* a_state = nullptr)
    {
        if (!a_actor || !a_player) {
            return false;
        }

        if (a_actor == a_player) {
            return false;
        }

        if (a_actor->IsDead() || a_actor->IsDisabled() || !a_actor->Get3D()) {
            if (a_state) {
                a_state->rejectedStateCount++;
            }
            return false;
        }

        const auto* actorCell = a_actor->GetParentCell();
        const auto* playerCell = a_player->GetParentCell();
        if (!actorCell || !playerCell || ((actorCell->IsInteriorCell() || playerCell->IsInteriorCell()) && actorCell != playerCell)) {
            if (a_state) {
                a_state->rejectedCellCount++;
            }
            return false;
        }

        if (GetDistanceSquared(a_actor->GetPosition(), a_player->GetPosition()) > a_radiusSquared) {
            if (a_state) {
                a_state->rejectedDistanceCount++;
            }
            return false;
        }

        return true;
    }

    bool DrawActorBumper(const RE::Actor* a_actor)
    {
        if (!a_actor) {
            return false;
        }

        VCD::Manager::ActorBumperContext context{};
        if (!VCD::Manager::GetSingleton().GetActorBumperContext(a_actor, context, a_actor == RE::PlayerCharacter::GetSingleton())) {
            return false;
        }

        RE::BSReadLockGuard lock(context.world->worldLock);

        auto* bumper = VCD::Manager::GetSingleton().FindWorldCharacterBumperShape(context.controller);
        if (!bumper) {
            return false;
        }

        RE::hkVector4 controllerPositionHK;
        context.controller->GetPosition(controllerPositionHK, false);
        const auto controllerPosition = VCD::ToNiPoint3(controllerPositionHK) * VCD::GetPresetScale();
        auto actorPosition = a_actor->GetPosition();
		const bool isPlayer = a_actor == RE::PlayerCharacter::GetSingleton();
        if (isPlayer) {
            actorPosition = controllerPosition;
        }
        else {
            actorPosition.z = controllerPosition.z;
        }

        RE::NiPoint3 vertexA = VCD::ToNiPoint3(bumper->vertexA) * VCD::GetPresetScale();
        RE::NiPoint3 vertexB = VCD::ToNiPoint3(bumper->vertexB) * VCD::GetPresetScale();
        float radius = bumper->radius * VCD::GetPresetScale();
        float yaw = -a_actor->data.angle.z;
        float c = std::cos(yaw);
        float s = std::sin(yaw);
        RE::NiPoint3 a = VCD::RotatePoint(vertexA, c, s) + actorPosition;
        RE::NiPoint3 b = VCD::RotatePoint(vertexB, c, s) + actorPosition;

        const auto& settings = Settings::GetSettings();
		const std::array<float, 4> color = isPlayer ? settings.drawColor : settings.drawNPCColor;
		const float lineThickness = isPlayer ? settings.drawLineThickness : settings.drawNPCLineThickness;
        DrawCapsule(a, b, radius, 1, VCD::ToNiColorA(color), lineThickness);
        return true;
    }

    void DrawSphere(RE::bhkSimpleShapePhantom* a_phantom, const RE::NiColorA& a_color, float a_lineThickness)
    {
         RE::NiPoint3 cameraPos = DebugAPI_IMPL::GetCameraPos();

         auto* playerCamera = RE::PlayerCamera::GetSingleton();
         if (!playerCamera) return;

         auto& rt = playerCamera->GetRuntimeData2();

         float yaw = rt.yaw;

         RE::NiPoint3 forward{
             std::cos(yaw),
             std::sin(yaw),
             0.0f
         };

         //push camera draw in front of player so they can actually see it
         RE::NiPoint3 projectedPos = cameraPos + forward * 100.0f;

        auto* sphere = VCD::Manager::GetSingleton().GetCameraPhantomShape(a_phantom);
        if (!sphere) return;

        const auto radius = sphere->radius * VCD::GetPresetScale();
        DebugAPI_IMPL::DebugAPI::GetSingleton()->DrawSphere(projectedPos, radius, 100, a_color, a_lineThickness);
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
            RE::NiPointer<RE::bhkSimpleShapePhantom> unk08;
        };

        auto* cameraPhantoms = reinterpret_cast<CameraCollisionPhantoms*>(cameraRTD.unk120);
        const auto& settings = Settings::GetSettings();
        const auto color = VCD::ToNiColorA(settings.drawCameraColor);

        DrawSphere(cameraPhantoms->unk00.get(), color, settings.drawCameraLineThickness);
        DrawSphere(cameraPhantoms->unk08.get(), color, settings.drawCameraLineThickness);
    }


    bool HasCachedActor(const NearbyActorDrawState& a_state, const RE::Actor* a_actor)
    {
        if (!a_actor) {
            return false;
        }

        for (auto& handle : a_state.handles) {
            auto actorPtr = handle.get();
            if (actorPtr.get() == a_actor) {
                return true;
            }
        }

        return false;
    }

    bool ScanActorHandles(const RE::BSTArray<RE::ActorHandle>& a_handles, int& a_bucketCount, const RE::PlayerCharacter* a_player, const float& a_radiusSquared)
    {
        auto& state = GetNearbyActorDrawState();
        const auto& settings = Settings::GetSettings();

        for (auto& handle : a_handles) {
            state.scannedCount++;
            a_bucketCount++;

            auto actorPtr = handle.get();
            auto* actor = actorPtr.get();

            if (!CanDrawActorBumper(actor, a_player, a_radiusSquared, &state) || HasCachedActor(state, actor)) {
                continue;
            }

            state.handles.push_back(handle);
            state.acceptedCount++;
            if (static_cast<int>(state.handles.size()) >= settings.nearbyActorDrawLimit) {
                state.limitReached = true;
                return true;
            }
        }

        return false;
    }

    void RefreshNearbyActorCache(const RE::PlayerCharacter* a_player)
    {
        auto& state = GetNearbyActorDrawState();
        state.handles.clear();
        state.scannedCount = 0;
        state.scannedHighCount = 0;
        state.scannedMiddleHighCount = 0;
        state.scannedMiddleLowCount = 0;
        state.acceptedCount = 0;
        state.rejectedStateCount = 0;
        state.rejectedCellCount = 0;
        state.rejectedDistanceCount = 0;
        state.limitReached = false;

        const auto& settings = Settings::GetSettings();
        if (!a_player || settings.nearbyActorDrawLimit <= 0) {
            logger::info("Nearby actor scan skipped. player={}, limit={}", a_player != nullptr, settings.nearbyActorDrawLimit);
            return;
        }

        auto* processLists = RE::ProcessLists::GetSingleton();
        if (!processLists) {
            logger::info("Nearby actor scan skipped. ProcessLists unavailable");
            return;
        }

        const auto radiusSquared = settings.nearbyActorDrawRadius * settings.nearbyActorDrawRadius;
        if (ScanActorHandles(processLists->highActorHandles, state.scannedHighCount, a_player, radiusSquared) ||
            ScanActorHandles(processLists->middleHighActorHandles, state.scannedMiddleHighCount, a_player, radiusSquared) ||
            ScanActorHandles(processLists->middleLowActorHandles, state.scannedMiddleLowCount, a_player, radiusSquared)) {
            LogNearbyActorScan(state, settings);
            return;
        }

        LogNearbyActorScan(state, settings);
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

        auto& state = GetNearbyActorDrawState();
        const auto& settings = Settings::GetSettings();
        const auto now = std::chrono::steady_clock::now();
        bool scanned = false;
        if (now >= state.nextScan) {
            RefreshNearbyActorCache(player);
            scanned = true;
            const auto interval = std::chrono::duration<float>(settings.nearbyActorScanInterval);
            state.nextScan = now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(interval);
        }

        int drawnCount = 0;
        int filteredCount = 0;
        const auto radiusSquared = settings.nearbyActorDrawRadius * settings.nearbyActorDrawRadius;
        for (auto& handle : state.handles) {
            auto actorPtr = handle.get();
            auto* actor = actorPtr.get();
            if (CanDrawActorBumper(actor, player, radiusSquared)) {
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

}
