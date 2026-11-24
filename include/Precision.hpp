
#pragma once

#include "PrecisionAPI.hpp"
#include "collision_layers.hpp"

namespace precision
{
    inline bool installed{ false };

    inline bool IsInstalled()
    {
        return installed;
    }

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

                auto* process = aggressor->GetHighProcess();
                auto attackData = process ? process->attackData : nullptr;

                const bool isLeftAttack = attackData && attackData->IsLeftAttack();

                if (CollisionLayers::ShouldRejectMeleeHit(aggressor, victim, 0, isLeftAttack)) {
                    callbackResult.bIgnoreHit = true;
                }

                logger::debug(
                    "Attacker: {}, AttackData: {}, LeftAttack: {}",
                    aggressor->GetName(),
                    attackData ? "Valid" : "Null",
                    isLeftAttack
                );

                return callbackResult;
            });

        installed = result == PRECISION_API::APIResult::OK;

        logger::info("Precision PreHitCallback registration: {}", installed);
    }
}
