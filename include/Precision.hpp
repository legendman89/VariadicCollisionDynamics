#pragma once

#include "PrecisionAPI.hpp"
#include "collision_layers.hpp"

inline void RegisterPrecisionCallbacks()
{
    auto* precision = static_cast<PRECISION_API::IVPrecision1*>(
        PRECISION_API::RequestPluginAPI(PRECISION_API::InterfaceVersion::V1));

    if (!precision) {
        logger::warn("Precision API unavailable");
        return;
    }

    const auto result = precision->AddPreHitCallback(
        SKSE::GetPluginHandle(),
        [](const PRECISION_API::PrecisionHitData& a_hitData) {
            PRECISION_API::PreHitCallbackReturn callbackResult;

            auto* aggressor = a_hitData.attacker;
            auto* victim = skyrim_cast<RE::Actor*>(a_hitData.target);

            if (!aggressor || !victim) {
                return callbackResult;
            }

            if (CollisionLayers::ShouldRejectMeleeHit(aggressor, victim, 0, false)) {
                callbackResult.bIgnoreHit = true;
            }

            return callbackResult;
        });

    logger::info("Precision PreHitCallback registration: {}", static_cast<int>(result));
}