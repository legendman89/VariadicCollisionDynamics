#pragma once
#include <CLibUtilsQTR/DrawDebug.hpp>
#include "helper.hpp"
#include "manager.hpp"
#include "settings.hpp"

namespace DebugAPI_IMPL {
    namespace DebugAPI_Ext {

        constexpr int32_t CAPSULE_SIDES = 16;

        // Exact port of TrueHUDMenu::DrawCircle(center, x, y, radius, segments, ...)
        inline void DrawCircleAxis(const RE::NiPoint3& center, const RE::NiPoint3& xAxis, const RE::NiPoint3& yAxis,
            float radius, uint32_t segments, int liftetimeMS = 10,
            const RE::NiColorA& color = { 1.0f, 0.0f, 0.0f, 1.0f }, float lineThickness = 1) {
            const float angleDelta = RE::NI_TWO_PI / segments;
            RE::NiPoint3 lastVertex = center + xAxis * radius;

            for (uint32_t i = 0; i < segments; i++) {
                RE::NiPoint3 vertex = center + (xAxis * cosf(angleDelta * (i + 1)) + yAxis * sinf(angleDelta * (i + 1))) * radius;
                DebugAPI::GetSingleton()->DrawLineForMS(lastVertex, vertex, liftetimeMS, color, lineThickness);
                lastVertex = vertex;
            }
        }

        // Exact port of TrueHUDMenu::DrawHalfCircle(center, x, y, radius, segments, ...)
        inline void DrawHalfCircle(const RE::NiPoint3& center, const RE::NiPoint3& xAxis, const RE::NiPoint3& yAxis,
            float radius, uint32_t segments, int liftetimeMS = 10,
            const RE::NiColorA& color = { 1.0f, 0.0f, 0.0f, 1.0f }, float lineThickness = 1) {
            const float angleDelta = RE::NI_TWO_PI / segments;
            RE::NiPoint3 lastVertex = center + xAxis * radius;

            for (uint32_t i = 0; i < segments / 2; i++) {
                RE::NiPoint3 vertex = center + (xAxis * cosf(angleDelta * (i + 1)) + yAxis * sinf(angleDelta * (i + 1))) * radius;
                DebugAPI::GetSingleton()->DrawLineForMS(lastVertex, vertex, liftetimeMS, color, lineThickness);
                lastVertex = vertex;
            }
        }

        // Exact port of TrueHUDMenu::DrawCapsule(vertexA, vertexB, radius, ...)
        inline void DrawCapsule(const RE::NiPoint3& vertexA, const RE::NiPoint3& vertexB, float radius,
            int liftetimeMS = 10, const RE::NiColorA& color = { 1.0f, 0.0f, 0.0f, 1.0f },
            float lineThickness = 1) {
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

        // Port of TrueHUDMenu::DrawCapsule(origin, halfHeight, radius, rotation, ...)
        inline void DrawCapsule(const RE::NiPoint3& origin, float halfHeight, float radius,
            const RE::NiQuaternion& rotation, int liftetimeMS = 10,
            const RE::NiColorA& color = { 1.0f, 0.0f, 0.0f, 1.0f }, float lineThickness = 1) {
            RE::NiPoint3 xAxis = RotateVector(rotation, RE::NiPoint3(1.0f, 0.0f, 0.0f));
            RE::NiPoint3 yAxis = RotateVector(rotation, RE::NiPoint3(0.0f, 1.0f, 0.0f));
            RE::NiPoint3 zAxis = RotateVector(rotation, RE::NiPoint3(0.0f, 0.0f, 1.0f));

            float halfAxis = max(halfHeight - radius, 1.0f);
            RE::NiPoint3 topEnd = origin + zAxis * halfAxis;
            RE::NiPoint3 bottomEnd = origin - zAxis * halfAxis;

            DrawCircleAxis(topEnd, xAxis, yAxis, radius, CAPSULE_SIDES, liftetimeMS, color, lineThickness);
            DrawCircleAxis(bottomEnd, xAxis, yAxis, radius, CAPSULE_SIDES, liftetimeMS, color, lineThickness);

            DrawHalfCircle(topEnd, yAxis, zAxis, radius, CAPSULE_SIDES, liftetimeMS, color, lineThickness);
            DrawHalfCircle(topEnd, xAxis, zAxis, radius, CAPSULE_SIDES, liftetimeMS, color, lineThickness);

            RE::NiPoint3 negZAxis = -zAxis;
            DrawHalfCircle(bottomEnd, yAxis, negZAxis, radius, CAPSULE_SIDES, liftetimeMS, color, lineThickness);
            DrawHalfCircle(bottomEnd, xAxis, negZAxis, radius, CAPSULE_SIDES, liftetimeMS, color, lineThickness);

            DebugAPI::GetSingleton()->DrawLineForMS(topEnd + xAxis * radius, bottomEnd + xAxis * radius, liftetimeMS, color, lineThickness);
            DebugAPI::GetSingleton()->DrawLineForMS(topEnd - xAxis * radius, bottomEnd - xAxis * radius, liftetimeMS, color, lineThickness);
            DebugAPI::GetSingleton()->DrawLineForMS(topEnd + yAxis * radius, bottomEnd + yAxis * radius, liftetimeMS, color, lineThickness);
            DebugAPI::GetSingleton()->DrawLineForMS(topEnd - yAxis * radius, bottomEnd - yAxis * radius, liftetimeMS, color, lineThickness);
        }

        inline void DrawPlayerBumper()
        {
            auto player = RE::PlayerCharacter::GetSingleton();
            if (!player)
                return;
            auto charController = skyrim_cast<RE::bhkCharProxyController*>(player->GetCharController());
            if (!charController)
                return;
            auto cell = player->GetParentCell();
            if (!cell)
                return;
            auto world = cell->GetbhkWorld();
            if (!world)
                return;

            RE::BSReadLockGuard lock(world->worldLock);
            auto* bumper = VCD::Manager::GetSingleton().FindWorldCharacterBumperShape(charController);
            if (!bumper)
                return;

            RE::hkVector4 controllerPosHK;
            charController->GetPosition(controllerPosHK, false);
            RE::NiPoint3 controllerPos = VCD::ToNiPoint3(controllerPosHK) * VCD::GetPresetScale();
            RE::NiPoint3 aLocal = VCD::ToNiPoint3(bumper->vertexA) * VCD::GetPresetScale();
            RE::NiPoint3 bLocal = VCD::ToNiPoint3(bumper->vertexB) * VCD::GetPresetScale();
            float radius = bumper->radius * VCD::GetPresetScale();

            float yaw = -player->data.angle.z;
            float c = std::cos(yaw);
            float s = std::sin(yaw);
            auto rotate = [&](const RE::NiPoint3& p) {
                return RE::NiPoint3(p.x * c - p.y * s, p.x * s + p.y * c, p.z);
            };

            RE::NiPoint3 a = rotate(aLocal) + controllerPos;
            RE::NiPoint3 b = rotate(bLocal) + controllerPos;

            using namespace DebugAPI_IMPL;

            auto* api = DebugAPI::GetSingleton();
            const auto& settings = Settings::GetSettings();
            DebugAPI_Ext::DrawCapsule(a, b, radius, 1, VCD::ToNiColorA(settings.drawColor), settings.drawLineThickness);
            api->Update();
        } 


    } 
}
