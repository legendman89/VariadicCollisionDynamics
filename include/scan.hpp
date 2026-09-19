#pragma once

#include "settings.hpp"

#include <chrono>
#include <string>
#include <vector>

namespace Scan {

    struct NearbyActorScanState
    {
        std::vector<RE::ActorHandle> handles{};
        std::chrono::steady_clock::time_point nextScan{};
        int scannedCount{ 0 };
        int scannedHighCount{ 0 };
        int scannedMiddleHighCount{ 0 };
        int scannedMiddleLowCount{ 0 };
        int acceptedCount{ 0 };
        int rejectedCount{ 0 };
        bool limitReached{ false };
        bool showUnregistered{ false };
    };

    struct NearbyActorScanOption
    {
        RE::ActorHandle handle{};
        std::string name{};
        std::string label{};
        RE::FormID formID{ 0 };
    };

    inline bool CompareNearbyActorLabels(const NearbyActorScanOption& a_left, const NearbyActorScanOption& a_right)
    {
        return a_left.label < a_right.label;
    }

    NearbyActorScanState& GetNearbyActorScanState();

    std::vector<NearbyActorScanOption> GetNearbyNPCActorOptions();

    void ResetNearbyActorScanState(NearbyActorScanState& a_state);

    bool HasCachedActor(const NearbyActorScanState& a_state, const RE::Actor* a_actor);

    bool CanScanNearbyActor(const RE::Actor* a_actor, const RE::PlayerCharacter* a_player, const float& a_radiusSquared, NearbyActorScanState* a_state = nullptr);

    bool ScanActorHandles(const RE::BSTArray<RE::ActorHandle>& a_handles, int& a_bucketCount, const RE::PlayerCharacter* a_player, const float& a_radiusSquared);

    void LogNearbyActorScan(const NearbyActorScanState& a_state, const Settings::VCDSettings& a_settings);

    void RefreshNearbyActorCache(const RE::PlayerCharacter* a_player);

    bool UpdateNearbyActorCache(const RE::PlayerCharacter* a_player, bool a_force = false);

}
