#include "posefixes_panel.hpp"
#include "dynamics.hpp"
#include "posefixes.hpp"
#include "scan.hpp"
#include "translate.hpp"

#include <chrono>
#include <unordered_map>
#include <vector>

namespace UI {

	bool ApplyPlayerPoseFixes(const bool& a_rebuildConvex)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return false;
		}

		auto& manager = VCD::Manager::GetSingleton();
		const auto poseFlags = PoseFixes::PlayerPose(player);
		if (player->IsSneaking()) {
			return manager.FixSneakingPose(player, poseFlags, false, a_rebuildConvex);
		}

		const auto* actorState = player->AsActorState();
		if (!actorState || !actorState->IsSitting()) {
			return false;
		}

		const auto& state = Dynamics::GetPresetState();
		if (state.applied) {
			return manager.SetPreset(player, state.current, poseFlags, false, a_rebuildConvex);
		}

		return manager.FixSittingPose(player, poseFlags, false);
	}

	bool ApplyNPCPoseFixActor(RE::Actor* a_actor, std::unordered_map<RE::FormID, bool>& a_seen, const bool& a_rebuildConvex)
	{
		if (!a_actor || a_seen[a_actor->GetFormID()]) {
			return false;
		}

		a_seen[a_actor->GetFormID()] = true;
		auto& manager = VCD::Manager::GetSingleton();
		const auto& settings = Settings::GetSettings();
		if (settings.enableNPCDynamics) {
			auto* player = RE::PlayerCharacter::GetSingleton();
			const auto radiusSquared = settings.nearbyActorScanRadius * settings.nearbyActorScanRadius;
			if (!Dynamics::CanApplyNPCDynamics(a_actor, player, radiusSquared)) {
				return false;
			}

			const char* stateName = "unknown";
			const auto preset = Dynamics::GetNPCPreset(a_actor, stateName);
			if (a_rebuildConvex) {
				return Dynamics::ApplyNPCPreset(a_actor, preset, stateName);
			}

			const auto* collisionData = Dynamics::GetNPCCollisionData(a_actor->GetFormID(), preset);
			if (!collisionData) {
				return false;
			}

			return manager.SetCollisionData(
				a_actor,
				*collisionData,
				preset,
				VCD::PresetName(preset),
				PoseFixes::NPCPose(a_actor),
				false,
				false
			);
		}

		const auto poseFlags = PoseFixes::NPCPose(a_actor);
		const auto applied = manager.FixSittingPose(a_actor, poseFlags, false);
		if (applied && poseFlags.isSitting) {
			Dynamics::TrackPoseFixedNPC(a_actor);
		}
		return applied;
	}

	bool ApplyNPCPoseFixHandle(const RE::ActorHandle& a_handle, std::unordered_map<RE::FormID, bool>& a_seen, const bool& a_rebuildConvex)
	{
		auto actorPtr = a_handle.get();
		return ApplyNPCPoseFixActor(actorPtr.get(), a_seen, a_rebuildConvex);
	}

	void ApplyNPCPoseFixHandles(const std::vector<RE::ActorHandle>& a_handles, std::unordered_map<RE::FormID, bool>& a_seen, const bool& a_rebuildConvex)
	{
		for (auto& handle : a_handles) {
			ApplyNPCPoseFixHandle(handle, a_seen, a_rebuildConvex);
		}
	}

	void ApplyNPCPoseFixes(const bool& a_rebuildConvex)
	{
		auto& npcState = Dynamics::GetNPCDynamicsState();
		if (Settings::GetSettings().enableNPCDynamics && a_rebuildConvex) {
			npcState.actors.clear();
		}

		std::unordered_map<RE::FormID, bool> seen{};
		if (!Settings::GetSettings().enableNPCDynamics) {
			ApplyNPCPoseFixHandles(npcState.poseFixedActors, seen, a_rebuildConvex);
		}

		ApplyNPCPoseFixHandles(npcState.nearbyActors, seen, a_rebuildConvex);
		ApplyNPCPoseFixHandles(Scan::GetNearbyActorScanState().handles, seen, a_rebuildConvex);
	}

	void ApplyPoseFixes(const bool& a_player, const bool& a_npc, const bool& a_rebuildConvex)
	{
		if (a_player) {
			ApplyPlayerPoseFixes(a_rebuildConvex);
		}

		if (a_npc) {
			ApplyNPCPoseFixes(a_rebuildConvex);
		}
	}

	void ApplyPendingPoseFixes()
	{
		auto& state = PoseFixes::GetPoseFixApplyState();
		const auto playerPending = state.playerPending;
		const auto npcPending = state.npcPending;
		state = {};
		ApplyPoseFixes(playerPending, npcPending, true);
	}

	void SchedulePoseFixApply(const bool& a_player, const bool& a_npc, const bool& a_waitForRelease)
	{
		if (!a_player && !a_npc) {
			return;
		}

		auto& state = PoseFixes::GetPoseFixApplyState();
		state.pending = true;
		state.playerPending |= a_player;
		state.npcPending |= a_npc;
		state.afterRelease |= a_waitForRelease && GUI::IsMouseDown(GUI::ImGuiMouseButton_Left);
		if (!state.afterRelease) {
			state.applyAt = std::chrono::steady_clock::now() + PoseFixes::kPoseFixApplyDebounce;
		}
	}

	void TickPoseFixApply()
	{
		auto& state = PoseFixes::GetPoseFixApplyState();
		if (!state.pending) {
			return;
		}

		const auto now = std::chrono::steady_clock::now();
		if (state.afterRelease) {
			if (GUI::IsMouseDown(GUI::ImGuiMouseButton_Left)) {
				return;
			}

			state.afterRelease = false;
			state.applyAt = now + PoseFixes::kPoseFixApplyDebounce;
			return;
		}

		if (now >= state.applyAt) {
			ApplyPendingPoseFixes();
		}
	}

	void ResetPoseFixesToDefaults()
	{
		Settings::ResetPoseFixes();
		PoseFixes::GetPoseFixApplyState() = {};
		ApplyPoseFixes(true, true, true);
	}

	void RenderPoseFixesSaveResetButtons()
	{
		if (IconCTAButton(Trans::Tr("Dynamics.PoseFix.SaveButton").c_str(), Settings::IsPoseFixesDirty(), Icons::kSave)) {
			Settings::SavePoseFixes();
		}
		Tooltip(Trans::Tr("Dynamics.PoseFix.SaveTooltip").c_str());

		GUI::SameLine(0.0F, 6.0F);

		if (IconCTAButton(Trans::Tr("Dynamics.PoseFix.DefaultButton").c_str(), !Settings::IsPoseFixesDefault(), Icons::kReset)) {
			ResetPoseFixesToDefaults();
		}
		Tooltip(Trans::Tr("Dynamics.PoseFix.DefaultTooltip").c_str());
	}

	void RenderPoseFixesActionBar()
	{
		const auto saveWidth = IconCTAButtonWidth(Trans::Tr("Dynamics.PoseFix.SaveButton").c_str());
		const auto resetWidth = IconCTAButtonWidth(Trans::Tr("Dynamics.PoseFix.DefaultButton").c_str());
		const auto rightGroupWidth = saveWidth + 6.0F + resetWidth;

		RenderActionBar(0.0F, 0.0F, rightGroupWidth, IconCTAButtonHeight(), [] {}, RenderPoseFixesSaveResetButtons);
	}

	void RenderPoseFixes()
	{
		auto& settings = Settings::GetSettings();

		if (GUI::BeginTable("PoseFixSettings", 2)) {
			GUI::TableSetupColumn(Trans::Tr("Dynamics.PoseFix.Column.Fix").c_str(), GUI::ImGuiTableColumnFlags_WidthFixed, kPoseFixesTableWidth);
			GUI::TableSetupColumn(Trans::Tr("Dynamics.PoseFix.Column.Scale").c_str(), GUI::ImGuiTableColumnFlags_WidthStretch);

			GUI::TableNextRow();
			GUI::TableNextColumn();
			const auto playerSittingChanged = GUI::Checkbox(Trans::Tr("Dynamics.PoseFix.PlayerSitting").c_str(), &settings.fixPlayerSitting);
			Tooltip(Trans::Tr("Dynamics.PoseFix.PlayerSittingTooltip").c_str());
			GUI::TableNextColumn();
			GUI::SetCursorPosX(GUI::GetCursorPosX() + 12.0F);
			GUI::BeginDisabled(!settings.fixPlayerSitting);
			GUI::SetNextItemWidth(kFixedPoseSliderWidth);
			const auto playerSittingScaleChanged = GUI::SliderFloat(Trans::Tr("Dynamics.PoseFix.PlayerSittingScale").c_str(), &settings.playerSittingScale, 0.3F, 1.0F);
			const auto playerSittingScaleActive = GUI::IsItemActive();
			GUI::EndDisabled();

			GUI::TableNextRow();
			GUI::TableNextColumn();
			const auto npcSittingChanged = GUI::Checkbox(Trans::Tr("Dynamics.PoseFix.NPCSitting").c_str(), &settings.fixNPCSitting);
			Tooltip(Trans::Tr("Dynamics.PoseFix.NPCSittingTooltip").c_str());
			GUI::TableNextColumn();
			GUI::SetCursorPosX(GUI::GetCursorPosX() + 12.0F);
			GUI::BeginDisabled(!settings.fixNPCSitting);
			GUI::SetNextItemWidth(kFixedPoseSliderWidth);
			const auto npcSittingScaleChanged = GUI::SliderFloat(Trans::Tr("Dynamics.PoseFix.NPCSittingScale").c_str(), &settings.npcSittingScale, 0.3F, 1.0F);
			const auto npcSittingScaleActive = GUI::IsItemActive();
			GUI::EndDisabled();

			GUI::TableNextRow();
			GUI::TableNextColumn();
			const auto dynamicCollisionAdjustmentInstalled = VCD::IsDynamicCollisionAdjustmentInstalled();
			GUI::BeginDisabled(dynamicCollisionAdjustmentInstalled);
			const auto sneakingFixChanged = GUI::Checkbox(Trans::Tr("Dynamics.PoseFix.PlayerSneaking").c_str(), &settings.fixPlayerSneaking);
			GUI::EndDisabled();
			GUI::TableNextColumn();
			GUI::SetCursorPosX(GUI::GetCursorPosX() + 12.0F);
			GUI::BeginDisabled(dynamicCollisionAdjustmentInstalled || !settings.fixPlayerSneaking);
			GUI::SetNextItemWidth(kFixedPoseSliderWidth);
			const auto sneakingScaleChanged = GUI::SliderFloat(Trans::Tr("Dynamics.PoseFix.PlayerSneakingScale").c_str(), &settings.playerSneakingScale, 0.3F, 1.0F);
			const auto sneakingScaleActive = GUI::IsItemActive();
			GUI::EndDisabled();

			GUI::TableNextRow();
			GUI::TableNextColumn();
			GUI::BeginDisabled(dynamicCollisionAdjustmentInstalled || !settings.enableNPCDynamics);
			const auto npcSneakingChanged = GUI::Checkbox(Trans::Tr("Dynamics.PoseFix.NPCSneaking").c_str(), &settings.fixNPCSneaking);
			GUI::EndDisabled();
			GUI::TableNextColumn();
			GUI::SetCursorPosX(GUI::GetCursorPosX() + 12.0F);
			GUI::BeginDisabled(dynamicCollisionAdjustmentInstalled || !settings.enableNPCDynamics || !settings.fixNPCSneaking);
			GUI::SetNextItemWidth(kFixedPoseSliderWidth);
			const auto npcSneakingScaleChanged = GUI::SliderFloat(Trans::Tr("Dynamics.PoseFix.NPCSneakingScale").c_str(), &settings.npcSneakingScale, 0.3F, 1.0F);
			const auto npcSneakingScaleActive = GUI::IsItemActive();
			GUI::EndDisabled();

			const auto playerChanged = playerSittingChanged || playerSittingScaleChanged || sneakingFixChanged || sneakingScaleChanged;
			const auto npcChanged = npcSittingChanged || npcSittingScaleChanged || npcSneakingChanged || npcSneakingScaleChanged;
			const auto waitForRelease = playerSittingScaleActive || npcSittingScaleActive || sneakingScaleActive || npcSneakingScaleActive;
			if (playerChanged || npcChanged) {
				ApplyPoseFixes(playerChanged, npcChanged, false);
				SchedulePoseFixApply(playerChanged, npcChanged, waitForRelease);
			}

			GUI::EndTable();
		}
	}

	void __stdcall RenderPoseFixesMenu()
	{
		TickPoseFixApply();

		RenderPoseFixesActionBar();

		GUI::ImVec2 scrollSize{};
		GUI::GetContentRegionAvail(&scrollSize);
		if (GUI::BeginChild("PoseFixesScrollRegion", scrollSize, GUI::ImGuiChildFlags_None, 0)) {
			RenderPoseFixes();
		}
		GUI::EndChild();
	}

}
