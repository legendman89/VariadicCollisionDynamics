
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

    inline PRECISION_API::PreHitCallbackReturn OnPreHit(const PRECISION_API::PrecisionHitData& a_hitData)
    {
        PRECISION_API::PreHitCallbackReturn result;
        auto* aggressor = a_hitData.attacker;
        auto* victim = skyrim_cast<RE::Actor*>(a_hitData.target);
        if (!aggressor || !victim) {
            return result;
        }

        auto* process = aggressor->GetHighProcess();
        const auto attackData = process ? process->attackData : nullptr;
        const bool isLeftAttack = attackData && attackData->IsLeftAttack();
        result.bIgnoreHit = CollisionLayers::ShouldRejectMeleeHit(aggressor, victim, 0, isLeftAttack);
        logger::debug("Attacker: {}, AttackData: {}, LeftAttack: {}", aggressor->GetName(), attackData ? "Valid" : "Null", isLeftAttack);
        return result;
    }

    inline void RegisterPrecisionCallbacks()
    {
        auto* precision = static_cast<PRECISION_API::IVPrecision1*>(PRECISION_API::RequestPluginAPI(PRECISION_API::InterfaceVersion::V1));

        if (!precision) {
            logger::warn("Precision API unavailable");
            return;
        }

        const auto result = precision->AddPreHitCallback(SKSE::GetPluginHandle(), OnPreHit);

        installed = result == PRECISION_API::APIResult::OK;

        logger::info("Precision PreHitCallback registration: {}", installed);
    }
}
