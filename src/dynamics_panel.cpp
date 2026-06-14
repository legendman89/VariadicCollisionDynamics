#include "dynamics_panel.hpp"
#include "tools_panel.hpp"

#include <iterator>
#include <string>

namespace UI {

    bool PresetCombo(const char* a_label, VCD::Preset& a_preset)
    {
        auto index = static_cast<int>(VCD::ToUnderlying(a_preset));
        if (!GUI::Combo(a_label, &index, VCD::kPresetNames, static_cast<int>(std::size(VCD::kPresetNames)))) {
            return false;
        }

        a_preset = static_cast<VCD::Preset>(index);
        return true;
    }

    void OpenPresetEditor(const VCD::Preset& a_preset)
    {
        auto& manager = VCD::Manager::GetSingleton();
        auto* presetConfig = manager.GetPresetConfig(a_preset);
        if (!presetConfig) {
            return;
        }

        auto& editor = GetPresetEditorState();
        if (editor.open) {
            StopPresetEditorPreview();
        }

        editor.open = true;
        editor.preview = Dynamics::IsPresetPreviewed(a_preset);
        editor.autoEnabledDraw = false;
        editor.previousDrawCollision = false;
        editor.preset = a_preset;
        editor.defaults = presetConfig->data;
        editor.current = presetConfig->data;

        if (editor.preview) {
            if (const auto* player = RE::PlayerCharacter::GetSingleton()) {
                Dynamics::StartPresetPreview(player, a_preset);
            }
        }
    }

    bool RefreshPresetEditor()
    {
        auto& editor = GetPresetEditorState();
        if (!editor.open) {
            return false;
        }

        auto* presetConfig = VCD::Manager::GetSingleton().GetPresetConfig(editor.preset);
        if (!presetConfig) {
            editor.open = false;
            return false;
        }

        editor.defaults = presetConfig->data;
        editor.current = presetConfig->data;
        return true;
    }

    void BeginAutoDraw()
    {
        auto& editor = GetPresetEditorState();
        auto& settings = Settings::GetSettings();
        if (!settings.autoDrawPreview || settings.drawCollision || editor.autoEnabledDraw) {
            return;
        }

        editor.previousDrawCollision = settings.drawCollision;
        editor.autoEnabledDraw = true;
        settings.drawCollision = true;
    }

    void EndAutoDraw()
    {
        auto& editor = GetPresetEditorState();
        if (!editor.autoEnabledDraw) {
            return;
        }

        Settings::GetSettings().drawCollision = editor.previousDrawCollision;
        editor.autoEnabledDraw = false;
        ClearDrawLines();
    }

    void StopPresetEditorPreview()
    {
        auto& editor = GetPresetEditorState();
        if (editor.preview) {
            if (const auto* player = RE::PlayerCharacter::GetSingleton()) {
                Dynamics::RestorePresetPreview(player);
            }
        }

        EndAutoDraw();
        editor.preview = false;
    }

    void ClosePresetEditor()
    {
        auto& editor = GetPresetEditorState();
        if (editor.preview) {
            Dynamics::SchedulePreviewRestore(Settings::GetSettings().previewRestoreDelay);
        }

        EndAutoDraw();
        editor.preview = false;
    }

    void UpdateEditedPreset()
    {
        auto& editor = GetPresetEditorState();
        auto& manager = VCD::Manager::GetSingleton();
        auto* presetConfig = manager.GetPresetConfig(editor.preset);
        if (!presetConfig) {
            return;
        }

        presetConfig->data = editor.current;
        presetConfig->data.RecalculateHeight();
        Settings::MarkPresetEdited(editor.preset, presetConfig->data);

        if (const auto* player = RE::PlayerCharacter::GetSingleton()) {
            if (Dynamics::IsPresetCurrent(editor.preset)) {
                BeginAutoDraw();
                manager.SetPreset(player, editor.preset);
            }
            else if (editor.preview) {
                BeginAutoDraw();
                Dynamics::StartPresetPreview(player, editor.preset);
            }
        }
    }

    void RenderStateRow(const char* a_label, VCD::Preset& a_preset)
    {
        GUI::TableNextRow();
        GUI::TableNextColumn();
        GUI::Text("%s", a_label);
        GUI::TableNextColumn();
        GUI::SetNextItemWidth(180.0F);
        if (PresetCombo((std::string("##") + a_label).c_str(), a_preset)) {
            PreviewPreset(a_preset);
        }
        GUI::SameLine();
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonHovered, Color::kEditHover);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonActive, Color::kEditActive);
        if (GUI::Button((std::string("Edit##") + a_label).c_str())) {
            OpenPresetEditor(a_preset);
        }
        Tooltip("Open sliders for the preset assigned to this state.");
        GUI::PopStyleColor(2);
    }

    void RenderPresetEditor()
    {
        auto& editor = GetPresetEditorState();
        if (!editor.open) {
            return;
        }

        auto& manager = VCD::Manager::GetSingleton();
        auto* presetConfig = manager.GetPresetConfig(editor.preset);
        if (!presetConfig) {
            editor.open = false;
            return;
        }

        const auto title = std::string("Edit Preset: ") + VCD::PresetName(editor.preset);
        constexpr auto windowSize = GUI::ImVec2{ 520.0F, 320.0F };
        GUI::SetNextWindowSize(windowSize, GUI::ImGuiCond_Appearing);
        if (const auto* io = GUI::GetIO()) {
            GUI::SetNextWindowPos(GUI::ImVec2{ io->DisplaySize.x * 0.5F, io->DisplaySize.y * 0.5F }, GUI::ImGuiCond_Appearing, GUI::ImVec2{ 0.5F, 0.5F });
        }

        const auto wasOpen = editor.open;
        if (!GUI::Begin(title.c_str(), &editor.open, 0)) {
            GUI::End();
            if (wasOpen && !editor.open) {
                ClosePresetEditor();
            }
            return;
        }

        const auto isCurrentPreset = Dynamics::IsPresetCurrent(editor.preset);
        if (!isCurrentPreset) {
            if (GUI::Checkbox("Preview This Preset", &editor.preview)) {
                if (const auto* player = RE::PlayerCharacter::GetSingleton()) {
                    if (editor.preview) {
                        BeginAutoDraw();
                        Dynamics::StartPresetPreview(player, editor.preset);
                    }
                    else {
                        Dynamics::RestorePresetPreview(player);
                        EndAutoDraw();
                    }
                }
            }
            Tooltip("Temporarily applies this preset for tuning. Closing the editor restores the current dynamics state after a short delay.");
        }

        GUI::PushItemWidth(260.0F);

        constexpr auto kOffsetLimit = 30.0F;
		constexpr auto kHeightLimit = 3.0F;

        auto radius = editor.current.capsule.radius;
        if (GUI::SliderFloat("Radius", &radius, 0.05F, 1.0F)) {
            editor.current.capsule.radius = radius;
            UpdateEditedPreset();
        }

        auto topOffset = editor.current.capsule.point1.z;
        if (GUI::SliderFloat("Top Offset", &topOffset, -kHeightLimit, kHeightLimit)) {
            editor.current.capsule.point1.z = topOffset;
            UpdateEditedPreset();
        }

        auto bottomOffset = editor.current.capsule.point2.z;
        if (GUI::SliderFloat("Bottom Offset", &bottomOffset, -kHeightLimit, kHeightLimit)) {
            editor.current.capsule.point2.z = bottomOffset;
            UpdateEditedPreset();
        }

        auto forward = editor.current.bump.translation.y;
        if (GUI::SliderFloat("Forward Offset", &forward, -kOffsetLimit, kOffsetLimit)) {
            editor.current.bump.translation.y = forward;
            UpdateEditedPreset();
        }

        auto side = editor.current.bump.translation.x;
        if (GUI::SliderFloat("Side Offset", &side, -kOffsetLimit, kOffsetLimit)) {
            editor.current.bump.translation.x = side;
            UpdateEditedPreset();
        }

        GUI::PopItemWidth();

        const auto* defaultPresetConfig = manager.GetDefaultPresetConfig(editor.preset);
        const bool differsFromDefault = defaultPresetConfig && !editor.current.IsSame(defaultPresetConfig->data);
        if (CTAButton("Default", differsFromDefault)) {
            if (defaultPresetConfig) {
                presetConfig->data = defaultPresetConfig->data;
                Settings::ClearPresetEdited(editor.preset);
                editor.defaults = presetConfig->data;
                editor.current = presetConfig->data;

                if (const auto* player = RE::PlayerCharacter::GetSingleton()) {
                    if (Dynamics::IsPresetCurrent(editor.preset)) {
                        BeginAutoDraw();
                        manager.SetPreset(player, editor.preset);
                    }
                    else if (editor.preview) {
                        BeginAutoDraw();
                        Dynamics::StartPresetPreview(player, editor.preset);
                    }
                }
            }
        }
        Tooltip("Restore this preset from the preset JSON defaults without saving it to disk.");

        GUI::End();
        if (wasOpen && !editor.open) {
            ClosePresetEditor();
        }
    }

    void __stdcall RenderDynamicsMenu()
    {
        auto& config = Dynamics::GetConfig();

        if (GUI::BeginTable("DynamicsPresetTable", 2)) {
            GUI::TableSetupColumn("State", GUI::ImGuiTableColumnFlags_WidthFixed, 120.0F);
            GUI::TableSetupColumn("Preset", GUI::ImGuiTableColumnFlags_WidthStretch);

            RenderStateRow("Outdoor", config.outdoor);
            RenderStateRow("Indoor", config.indoor);
            RenderStateRow("Combat", config.combat);
            RenderStateRow("Werewolf", config.werewolf);
            RenderStateRow("Vampire Lord", config.vampireLord);
            RenderStateRow("Neutral", config.neutral);

            GUI::EndTable();
        }

        GUI::Separator();

        if (CTAButton("Save Settings", Settings::IsDirty())) {
            Settings::Save();
        }
        Tooltip("Save state mappings and edited presets to disk.");

        GUI::SameLine();

        const Settings::VCDSettings defaults{};
        const bool changed = !Settings::SettingsEqual(Settings::GetSettings(), defaults);

        if (CTAButton("Default", changed)) {
            Settings::GetSettings() = defaults;
            Settings::ApplySettings(Settings::GetSettings());

            if (RefreshPresetEditor()) {
                PreviewPreset(GetPresetEditorState().preset);
            }
        }
        Tooltip("Reset tools visualization, logging, state mappings, and preset overrides to defaults.");

        RenderPresetEditor();
    }

}
