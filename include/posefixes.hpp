#pragma once

#include "helper.hpp"
#include "settings.hpp"

#include <chrono>

namespace PoseFixes {

    inline constexpr auto kPoseFixApplyDebounce = std::chrono::milliseconds(250);

    struct PoseFixApplyState
    {
        bool pending{ false };
        bool afterRelease{ false };
        bool playerPending{ false };
        bool npcPending{ false };
        std::chrono::steady_clock::time_point applyAt{};
    };

    inline PoseFixApplyState& GetPoseFixApplyState()
    {
        static PoseFixApplyState state{};
        return state;
    }

    inline bool IsSittingFurnitureName(const char* a_name)
    {
        if (!a_name || a_name[0] == '\0') {
            return false;
        }

        const std::string_view name(a_name);
        return VCD::ContainsInsensitive(name, "bench") ||
            VCD::ContainsInsensitive(name, "chair") ||
            VCD::ContainsInsensitive(name, "stool") ||
            VCD::ContainsInsensitive(name, "throne") ||
            VCD::ContainsInsensitive(name, "grindstone") ||
            VCD::ContainsInsensitive(name, "tanning rack");
    }

    inline bool IsGrindstoneFurnitureName(const char* a_name)
    {
        return a_name && VCD::ContainsInsensitive(a_name, "grindstone");
    }

    inline const RE::TESIdleForm* GetSittingIdle(const RE::Actor* a_actor)
    {
        if (!a_actor) {
            return nullptr;
        }

        const auto* highProcess = a_actor->GetHighProcess();
        if (highProcess && highProcess->currentProcessIdle) {
            return highProcess->currentProcessIdle;
        }

        const auto* process = a_actor->GetActorRuntimeData().currentProcess;
        const auto* middleHigh = process ? process->middleHigh : nullptr;
        return middleHigh ? middleHigh->furnitureIdle : nullptr;
    }

    inline bool IsChildSittingOnKnees(const RE::Actor* a_actor)
    {
        const auto* idle = GetSittingIdle(a_actor);
        if (!idle) {
            return false;
        }

        const auto* editorID = idle->GetFormEditorID();
        const auto* eventName = idle->animEventName.c_str();
        return (editorID && VCD::ContainsInsensitive(editorID, "ChildSitOnKnees")) ||
            (eventName && VCD::ContainsInsensitive(eventName, "ChildSitOnKnees"));
    }

    inline VCD::PoseFlags GetPoseFlags(const RE::Actor* a_actor)
    {
        VCD::PoseFlags poseFlags{};
        poseFlags.isSneaking = a_actor && a_actor->IsSneaking();

        const auto* actorState = a_actor ? a_actor->AsActorState() : nullptr;
        if (!actorState || !actorState->IsSitting()) {
            return poseFlags;
        }

        const auto* furnitureName = VCD::GetOccupiedFurnitureName(a_actor);
        poseFlags.isGrindstone = IsGrindstoneFurnitureName(furnitureName);
        if (IsSittingFurnitureName(furnitureName)) {
            poseFlags.isSitting = true;
            return poseFlags;
        }

        poseFlags.isChildSittingOnKnees = IsChildSittingOnKnees(a_actor);
        poseFlags.isSitting = poseFlags.isChildSittingOnKnees;
        return poseFlags;
    }

    inline float GetSittingScale(const RE::Actor* a_actor, const VCD::PoseFlags& a_poseFlags)
    {
        const auto& settings = Settings::GetSettings();
        if (a_poseFlags.isGrindstone) {
            return settings.grindstoneSittingScale;
        }

        return a_actor == RE::PlayerCharacter::GetSingleton() ? settings.playerSittingScale : settings.npcSittingScale;
    }

    inline bool IsReallySitting(const RE::Actor* a_actor)
    {
        return GetPoseFlags(a_actor).isSitting;
    }

	inline VCD::PoseFlags PlayerPose(const RE::Actor* a_actor)
	{
		const auto& settings = Settings::GetSettings();
		auto poseFlags = GetPoseFlags(a_actor);
		if (!settings.fixPlayerSitting) {
			poseFlags.isSitting = false;
			poseFlags.isChildSittingOnKnees = false;
			poseFlags.isGrindstone = false;
		}
		poseFlags.isSneaking = settings.fixPlayerSneaking && poseFlags.isSneaking;
		return poseFlags;
	}

	inline VCD::PoseFlags NPCPose(const RE::Actor* a_actor)
	{
		const auto& settings = Settings::GetSettings();
		auto poseFlags = GetPoseFlags(a_actor);
		if (!settings.fixNPCSitting) {
			poseFlags.isSitting = false;
			poseFlags.isChildSittingOnKnees = false;
			poseFlags.isGrindstone = false;
		}
		poseFlags.isSneaking = settings.enableNPCDynamics && settings.fixNPCSneaking && poseFlags.isSneaking;
		return poseFlags;
	}

}
