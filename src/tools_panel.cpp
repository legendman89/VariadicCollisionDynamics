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

        GUI::Text(Trans::Tr("Tools.CurrentDynamics.CurrentState").c_str(),
            state.stateName);

        GUI::Text(Trans::Tr("Tools.CurrentDynamics.CurrentPreset").c_str(),
            state.applied ? VCD::PresetName(state.current) : Trans::Tr("Common.Unknown").c_str());

        if (preview.active) {
            GUI::Text(Trans::Tr("Tools.CurrentDynamics.PreviewPreset").c_str(),
                VCD::PresetName(preview.preset));
        }
    }

    void RenderVisualization()
    {
        auto& settings = Settings::GetSettings();

        if (GUI::Checkbox(Trans::Tr("Tools.Visualization.DrawPlayerCollision").c_str(), &settings.drawCollision)) {
            ClearDrawLines();
        }

        if (GUI::Checkbox(Trans::Tr("Tools.Visualization.DrawNearbyActors").c_str(), &settings.drawNearbyActors)) {
            ClearDrawLines();
        }

        if (GUI::Checkbox(Trans::Tr("Tools.Visualization.DrawCameraCollision").c_str(), &settings.drawCameraCollision)) {
            ClearDrawLines();
        }

        GUI::SetNextItemWidth(260.0F);
        GUI::SliderFloat(Trans::Tr("Tools.Visualization.NearbyActorRadius").c_str(),
            &settings.nearbyActorDrawRadius, 256.0F, 8192.0F);

        GUI::SetNextItemWidth(260.0F);
        GUI::SliderFloat(Trans::Tr("Tools.Visualization.NearbyActorScanInterval").c_str(),
            &settings.nearbyActorScanInterval, 0.1F, 2.0F);

        GUI::SetNextItemWidth(260.0F);
        GUI::SliderInt(Trans::Tr("Tools.Visualization.NearbyActorLimit").c_str(),
            &settings.nearbyActorDrawLimit, 1, 64);

        GUI::Checkbox(Trans::Tr("Tools.Visualization.AutoDrawWhileEditing").c_str(),
            &settings.autoDrawPreview);

        WrappedTooltip(Trans::Tr("Tools.Visualization.AutoDrawWhileEditing.Tooltip").c_str());

        GUI::SetNextItemWidth(260.0F);
        GUI::ColorEdit4(Trans::Tr("Tools.Visualization.PlayerLineColor").c_str(),
            settings.drawColor.data(),
            GUI::ImGuiColorEditFlags_DisplayRGB | GUI::ImGuiColorEditFlags_AlphaBar);

        GUI::SetNextItemWidth(260.0F);
        GUI::ColorEdit4(Trans::Tr("Tools.Visualization.NPCLineColor").c_str(),
            settings.drawNPCColor.data(),
            GUI::ImGuiColorEditFlags_DisplayRGB | GUI::ImGuiColorEditFlags_AlphaBar);

        GUI::SetNextItemWidth(260.0F);
        GUI::SliderFloat(Trans::Tr("Tools.Visualization.PlayerLineThickness").c_str(),
            &settings.drawLineThickness, 0.25F, 5.0F);

        GUI::SetNextItemWidth(260.0F);
        GUI::SliderFloat(Trans::Tr("Tools.Visualization.NPCLineThickness").c_str(),
            &settings.drawNPCLineThickness, 0.25F, 5.0F);

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

        if (CTAButton(Trans::Tr("Tools.Save.Button").c_str(), Settings::IsToolsDirty())) {
            Settings::SaveTools();
        }

        Tooltip(Trans::Tr("Tools.Save.Tooltip").c_str());

        GUI::SameLine();

        if (CTAButton(Trans::Tr("Tools.Default.Button").c_str(), !Settings::IsToolsDefault())) {
            Settings::ResetTools();
            ClearDrawLines();
        }

        Tooltip(Trans::Tr("Tools.Default.Tooltip").c_str());
    }

}
