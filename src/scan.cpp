
#include "dynamics.hpp"
#include "helper.hpp"
#include "logger.hpp"
#include "scan.hpp"

#include <algorithm>
#include <format>
#include <string>
#include <unordered_map>

namespace Scan {

    NearbyActorScanState& GetNearbyActorScanState()
    {
        static NearbyActorScanState state{};
        return state;
    }

    void ResetNearbyActorScanState(NearbyActorScanState& a_state)
    {
        a_state.handles.clear();
        a_state.scannedCount = 0;
        a_state.scannedHighCount = 0;
        a_state.scannedMiddleHighCount = 0;
        a_state.scannedMiddleLowCount = 0;
        a_state.acceptedCount = 0;
        a_state.rejectedCount = 0;
        a_state.limitReached = false;
    }

    bool HasCachedActor(const NearbyActorScanState& a_state, const RE::Actor* a_actor)
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

    bool CanScanNearbyActor(const RE::Actor* a_actor, const RE::PlayerCharacter* a_player, const float& a_radiusSquared, NearbyActorScanState* a_state)
    {
        if (!Dynamics::CanApplyNPCDynamics(const_cast<RE::Actor*>(a_actor), a_player, a_radiusSquared)) {
            if (a_state) {
                a_state->rejectedCount++;
            }
            return false;
        }

        return true;
    }

    bool ScanActorHandles(const RE::BSTArray<RE::ActorHandle>& a_handles, int& a_bucketCount, const RE::PlayerCharacter* a_player, const float& a_radiusSquared)
    {
        auto& state = GetNearbyActorScanState();
        const auto& settings = Settings::GetSettings();

        for (auto& handle : a_handles) {
            state.scannedCount++;
            a_bucketCount++;

            auto actorPtr = handle.get();
            auto* actor = actorPtr.get();

            if (!CanScanNearbyActor(actor, a_player, a_radiusSquared, &state) || HasCachedActor(state, actor)) {
                continue;
            }

            state.handles.push_back(handle);
            state.acceptedCount++;
            if (static_cast<int>(state.handles.size()) >= settings.nearbyActorScanLimit) {
                state.limitReached = true;
                return true;
            }
        }

        return false;
    }

    void LogNearbyActorScan(const NearbyActorScanState& a_state, const Settings::VCDSettings& a_settings)
    {
        logger::trace("Nearby actor scan: scanned={} (high={}, middleHigh={}, middleLow={}), accepted={}, rejected={}, radius={}, limit={}, limitReached={}",
            a_state.scannedCount,
            a_state.scannedHighCount,
            a_state.scannedMiddleHighCount,
            a_state.scannedMiddleLowCount,
            a_state.acceptedCount,
            a_state.rejectedCount,
            a_settings.nearbyActorScanRadius,
            a_settings.nearbyActorScanLimit,
            a_state.limitReached);
    }

    void RefreshNearbyActorCache(const RE::PlayerCharacter* a_player)
    {
        auto& state = GetNearbyActorScanState();
        ResetNearbyActorScanState(state);

        const auto& settings = Settings::GetSettings();
        if (!a_player || settings.nearbyActorScanLimit <= 0) {
            logger::info("Nearby actor scan skipped. player={}, limit={}", a_player != nullptr, settings.nearbyActorScanLimit);
            return;
        }

        auto* processLists = RE::ProcessLists::GetSingleton();
        if (!processLists) {
            logger::info("Nearby actor scan skipped. ProcessLists unavailable");
            return;
        }

        const auto radiusSquared = settings.nearbyActorScanRadius * settings.nearbyActorScanRadius;
        if (ScanActorHandles(processLists->highActorHandles, state.scannedHighCount, a_player, radiusSquared) ||
            ScanActorHandles(processLists->middleHighActorHandles, state.scannedMiddleHighCount, a_player, radiusSquared) ||
            ScanActorHandles(processLists->middleLowActorHandles, state.scannedMiddleLowCount, a_player, radiusSquared)) {
            LogNearbyActorScan(state, settings);
            return;
        }

        LogNearbyActorScan(state, settings);
    }

    std::vector<NearbyActorScanOption> GetNearbyNPCActorOptions()
    {
        auto& scanState = ::Scan::GetNearbyActorScanState();
        auto& npcState = Dynamics::GetNPCDynamicsState();

        std::vector<NearbyActorScanOption> options{};
        std::unordered_map<std::string, int> nameCounts{};
        std::unordered_map<RE::FormID, bool> seen{};

        auto addNPCInfo = [&](const RE::ActorHandle& a_handle) {
            auto actorPtr = a_handle.get();
            auto* actor = actorPtr.get();
            if (!actor || seen[actor->GetFormID()]) {
                return;
            }

            seen[actor->GetFormID()] = true;
            const auto name = VCD::GetActorName(actor);
            nameCounts[name]++;
            options.push_back({ a_handle, name, name, actor->GetFormID() });
        };

        for (auto& handle : npcState.nearbyActors) {
            addNPCInfo(handle);
        }

        for (auto& handle : scanState.handles) {
            auto actorPtr = handle.get();
            auto* actor = actorPtr.get();
            if (!VCD::Race::IsSupportedNPCPresetActor(actor)) {
                continue;
            }

            addNPCInfo(handle);
        }

        for (auto& option : options) {
            if (nameCounts[option.name] > 1) {
                option.label = std::format("{} [{:08X}]", option.name, option.formID);
            }
        }

        std::sort(options.begin(), options.end(),
            [](const NearbyActorScanOption& a_left, const NearbyActorScanOption& a_right) {
                return a_left.label < a_right.label;
            }
        );

        return options;
    }

}
