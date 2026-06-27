#pragma once

#include "settings.hpp"
#include "logger.hpp"

namespace DebugAPI_IMPL::Draw {

    inline constexpr int32_t CAPSULE_SIDES = 16;

    struct NearbyActorDrawState
    {
        std::vector<RE::ActorHandle> handles{};
        std::chrono::steady_clock::time_point nextScan{};
        int scannedCount{ 0 };
        int scannedHighCount{ 0 };
        int scannedMiddleHighCount{ 0 };
        int scannedMiddleLowCount{ 0 };
        int acceptedCount{ 0 };
        int rejectedStateCount{ 0 };
        int rejectedCellCount{ 0 };
        int rejectedDistanceCount{ 0 };
        bool limitReached{ false };
    };

    inline NearbyActorDrawState& GetNearbyActorDrawState()
    {
        static NearbyActorDrawState state{};
        return state;
    }

    inline float GetDistanceSquared(const RE::NiPoint3& a_left, const RE::NiPoint3& a_right)
    {
        const auto x = a_left.x - a_right.x;
        const auto y = a_left.y - a_right.y;
        const auto z = a_left.z - a_right.z;
        return (x * x) + (y * y) + (z * z);
    }

    inline void LogNearbyActorScan(const NearbyActorDrawState& a_state, const Settings::VCDSettings& a_settings)
    {
        logger::debug("Nearby actor scan: scanned={} (high={}, middleHigh={}, middleLow={}), accepted={}, rejected(state={}, cell={}, distance={}), radius={}, limit={}, limitReached={}",
            a_state.scannedCount,
            a_state.scannedHighCount,
            a_state.scannedMiddleHighCount,
            a_state.scannedMiddleLowCount,
            a_state.acceptedCount,
            a_state.rejectedStateCount,
            a_state.rejectedCellCount,
            a_state.rejectedDistanceCount,
            a_settings.nearbyActorDrawRadius,
            a_settings.nearbyActorDrawLimit,
            a_state.limitReached);
    }

    void DrawPlayerBumper();

    void DrawNearbyActorBumpers();

    void DrawCameraBumper();

}
