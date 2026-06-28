#include "tools_panel.hpp"

#include <CLibUtilsQTR/DrawDebug.hpp>
#include <iterator>
#include "translate.hpp"

namespace UI {

    void ClearDrawLines()
    {
        auto api = DebugAPI_IMPL::DebugAPI::GetSingleton();
        api->LinesToDraw.clear();
        api->Update();
    }

    void ApplyLogLevel(const int& a_level)
    {
        const auto level = static_cast<spdlog::level::level_enum>(Settings::NormalizeLogLevel(a_level));
        spdlog::set_level(level);
        spdlog::flush_on(level);
    }

    void RenderReadout()
    {
        const auto& state = Dynamics::GetPresetState();
        const auto& preview = Dynamics::GetPreviewState();

        GUI::Text(Trans::Tr("Tools.CurrentDynamics.CurrentState").c_str(), state.stateName);

        GUI::Text(Trans::Tr("Tools.CurrentDynamics.CurrentPreset").c_str(),
            state.applied ? VCD::PresetName(state.current) : Trans::Tr("Common.Unknown").c_str());

        if (preview.active) {
            GUI::Text(Trans::Tr("Tools.CurrentDynamics.PreviewPreset").c_str(), VCD::PresetName(preview.preset));
        }
    }

    void RenderVisualization()
    {
        auto& settings = Settings::GetSettings();
        constexpr auto colorEditFlags = GUI::ImGuiColorEditFlags_DisplayRGB | GUI::ImGuiColorEditFlags_AlphaBar;

        if (GUI::Checkbox(Trans::Tr("Tools.Visualization.DrawPlayerCollision").c_str(), &settings.drawCollision)) {
            ClearDrawLines();
        }

        if (GUI::Checkbox(Trans::Tr("Tools.Visualization.DrawNearbyActors").c_str(), &settings.drawNearbyActors)) {
            ClearDrawLines();
        }

        GUI::SetNextItemWidth(260.0F);
        GUI::SliderFloat(Trans::Tr("Tools.Visualization.NearbyActorRadius").c_str(), &settings.nearbyActorDrawRadius, 256.0F, 8192.0F);

        GUI::SetNextItemWidth(260.0F);
        GUI::SliderFloat(Trans::Tr("Tools.Visualization.NearbyActorScanInterval").c_str(), &settings.nearbyActorScanInterval, 0.1F, 2.0F);

        GUI::SetNextItemWidth(260.0F);
        GUI::SliderInt(Trans::Tr("Tools.Visualization.NearbyActorLimit").c_str(), &settings.nearbyActorDrawLimit, 1, 64);

        GUI::Checkbox(Trans::Tr("Tools.Visualization.AutoDrawWhileEditing").c_str(), &settings.autoDrawPreview);

        WrappedTooltip(Trans::Tr("Tools.Visualization.AutoDrawWhileEditing.Tooltip").c_str());

        const auto playerLine = Trans::Tr("Tools.Visualization.Line.Player");
        const auto npcLine = Trans::Tr("Tools.Visualization.Line.NPC");
        const auto colorLabel = Trans::Tr("Tools.Visualization.Line.Color");
        const auto thicknessLabel = Trans::Tr("Tools.Visualization.Line.Thickness");

        GUI::ImVec2 playerLineSize{};
        GUI::ImVec2 npcLineSize{};
        GUI::ImVec2 colorLabelSize{};
        GUI::ImVec2 thicknessLabelSize{};

        GUI::CalcTextSize(&playerLineSize, playerLine.c_str(), nullptr, false, -1.0F);
        GUI::CalcTextSize(&npcLineSize, npcLine.c_str(), nullptr, false, -1.0F);
        GUI::CalcTextSize(&colorLabelSize, colorLabel.c_str(), nullptr, false, -1.0F);
        GUI::CalcTextSize(&thicknessLabelSize, thicknessLabel.c_str(), nullptr, false, -1.0F);

        const auto lineLabelWidth = (playerLineSize.x > npcLineSize.x ? playerLineSize.x : npcLineSize.x) + 12.0F;
        const auto colorControlWidth = colorLabelSize.x + 12.0F > 260.0F ? colorLabelSize.x + 12.0F : 260.0F;
        const auto thicknessControlWidth = thicknessLabelSize.x + 12.0F > 260.0F ? thicknessLabelSize.x + 12.0F : 260.0F;

        if (GUI::BeginTable("LineVisualizationTable", 3, GUI::ImGuiTableFlags_SizingFixedFit | GUI::ImGuiTableFlags_NoSavedSettings)) {
            GUI::TableSetupColumn("Line", GUI::ImGuiTableColumnFlags_WidthFixed, lineLabelWidth);
            GUI::TableSetupColumn("Color", GUI::ImGuiTableColumnFlags_WidthFixed, colorControlWidth);
            GUI::TableSetupColumn("Thickness", GUI::ImGuiTableColumnFlags_WidthFixed, thicknessControlWidth);

            GUI::TableNextRow();
            GUI::TableNextColumn();
            GUI::TableNextColumn();
            GUI::AlignTextToFramePadding();
            GUI::TextUnformatted(colorLabel.c_str());
            GUI::TableNextColumn();
            GUI::AlignTextToFramePadding();
            GUI::TextUnformatted(thicknessLabel.c_str());

            GUI::TableNextRow();
            GUI::TableNextColumn();
            GUI::AlignTextToFramePadding();
            GUI::TextUnformatted(playerLine.c_str());
            GUI::TableNextColumn();
            GUI::SetNextItemWidth(colorControlWidth);
            GUI::ColorEdit4("##PlayerLineColor", settings.drawColor.data(), colorEditFlags);
            GUI::TableNextColumn();
            GUI::SetNextItemWidth(thicknessControlWidth);
            GUI::SliderFloat("##PlayerLineThickness", &settings.drawLineThickness, 0.25F, 5.0F);

            GUI::TableNextRow();
            GUI::TableNextColumn();
            GUI::AlignTextToFramePadding();
            GUI::TextUnformatted(npcLine.c_str());
            GUI::TableNextColumn();
            GUI::SetNextItemWidth(colorControlWidth);
            GUI::ColorEdit4("##NPCLineColor", settings.drawNPCColor.data(), colorEditFlags);
            GUI::TableNextColumn();
            GUI::SetNextItemWidth(thicknessControlWidth);
            GUI::SliderFloat("##NPCLineThickness", &settings.drawNPCLineThickness, 0.25F, 5.0F);

            GUI::EndTable();
        }

        GUI::Spacing();

        GUI::SetNextItemWidth(260.0F);
        GUI::SliderFloat(Trans::Tr("Tools.Visualization.PreviewRestoreDelay").c_str(),
            &settings.previewRestoreDelay, 0.0F, 10.0F);

        Tooltip(Trans::Tr("Tools.Visualization.PreviewRestoreDelay.Tooltip").c_str());
    }

    void RenderLogging()
    {
        auto& settings = Settings::GetSettings();
        auto logLevel = Settings::NormalizeLogLevel(settings.logLevel);

        GUI::SetNextItemWidth(180.0F);
        SolidBackground(GUI::ImGuiCol_PopupBg);

        if (GUI::Combo(Trans::Tr("Tools.Logging.LogLevel").c_str(),
            &logLevel,
            kLogLevels,
            static_cast<int>(std::size(kLogLevels)))) {
            settings.logLevel = logLevel;
            ApplyLogLevel(settings.logLevel);
        }

        GUI::PopStyleColor();
    }

    void __stdcall RenderToolsMenu()
    {
        constexpr auto defaultOpen = GUI::ImGuiTreeNodeFlags_DefaultOpen;

        if (GUI::CollapsingHeader(Trans::Tr("Tools.CurrentDynamics.Header").c_str(), defaultOpen)) {
            RenderReadout();
        }

        GUI::Spacing();

        if (GUI::CollapsingHeader(Trans::Tr("Tools.Visualization.Header").c_str(), defaultOpen)) {
            RenderVisualization();
        }

        GUI::Spacing();

        if (GUI::CollapsingHeader(Trans::Tr("Tools.Logging.Header").c_str(), defaultOpen)) {
            RenderLogging();
        }

        GUI::Spacing();

        if (IconCTAButton(Trans::Tr("Tools.Save.Button").c_str(), Settings::IsToolsDirty(), Icons::kSave)) {
            Settings::SaveTools();
        }

        Tooltip(Trans::Tr("Tools.Save.Tooltip").c_str());

        GUI::SameLine();

        if (IconCTAButton(Trans::Tr("Tools.Default.Button").c_str(), !Settings::IsToolsDefault(), Icons::kReset)) {
            Settings::ResetTools();
            ClearDrawLines();
        }

        Tooltip(Trans::Tr("Tools.Default.Tooltip").c_str());
    }

}
