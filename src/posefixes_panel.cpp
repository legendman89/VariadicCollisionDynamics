#include "posefixes_panel.hpp"
#include "dynamics.hpp"
#include "posefixes.hpp"
#include "translate.hpp"

namespace UI {

	void ResetPoseFixesToDefaults()
	{
		Settings::ResetPoseFixes();
		Dynamics::GetNPCDynamicsState().actors.clear();

		if (auto* player = RE::PlayerCharacter::GetSingleton()) {
			const auto poseFlags = PoseFixes::PlayerPose(player);
			auto& manager = VCD::Manager::GetSingleton();

			if (player->IsSneaking()) {
				manager.FixSneakingPose(player, poseFlags, false);
			}
			else if (poseFlags.isSitting) {
				const auto& state = Dynamics::GetPresetState();
				if (state.applied) {
					manager.SetPreset(player, state.current, poseFlags, false);
				}
				else {
					manager.FixSittingPose(player, poseFlags, false);
				}
			}
		}
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
			GUI::EndDisabled();

			if (auto* player = RE::PlayerCharacter::GetSingleton(); player && player->IsSneaking()) {
				auto& manager = VCD::Manager::GetSingleton();
				const auto poseFlags = PoseFixes::PlayerPose(player);
				if (sneakingFixChanged || sneakingScaleChanged) {
					manager.FixSneakingPose(player, poseFlags, false);
				}
			}

			if (playerSittingChanged || playerSittingScaleChanged) {
				auto* player = RE::PlayerCharacter::GetSingleton();
				const auto* actorState = player ? player->AsActorState() : nullptr;
				if (actorState && actorState->IsSitting()) {
					const auto poseFlags = PoseFixes::PlayerPose(player);
					const auto& state = Dynamics::GetPresetState();
					if (state.applied) {
						VCD::Manager::GetSingleton().SetPreset(player, state.current, poseFlags, false);
					}
					else {
						VCD::Manager::GetSingleton().FixSittingPose(player, poseFlags, false);
					}
				}
			}

			if (npcSittingChanged || npcSittingScaleChanged || npcSneakingChanged || npcSneakingScaleChanged) {
				Dynamics::GetNPCDynamicsState().actors.clear();
			}

			GUI::EndTable();
		}
	}

	void __stdcall RenderPoseFixesMenu()
	{
		RenderPoseFixesActionBar();

		GUI::ImVec2 scrollSize{};
		GUI::GetContentRegionAvail(&scrollSize);
		if (GUI::BeginChild("PoseFixesScrollRegion", scrollSize, GUI::ImGuiChildFlags_None, 0)) {
			RenderPoseFixes();
		}
		GUI::EndChild();
	}

}
