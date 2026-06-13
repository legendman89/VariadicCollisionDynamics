#include "menu.hpp"
#include "button.hpp"
#include "color.hpp"
#include "dynamics.hpp"
#include "logger.hpp"
#include "manager.hpp"
#include "preset.hpp"
#include "settings.hpp"

#include <iterator>
#include <string>

namespace UI {

    namespace DynamicsPanel {
        constexpr const char* kPresetNames[] = {
            "Vanilla-like",
            "Personal Space",
            "Compact",
            "Bulky"
        };

        struct PresetEditorState
        {
            bool open{ false };
            VCD::Preset preset{ VCD::Preset::kVanillaLike };
            VCD::CollisionData defaults{};
            VCD::CollisionData current{};
        };

        PresetEditorState& GetPresetEditorState()
        {
            static PresetEditorState state{};
            return state;
        }

        bool PresetCombo(const char* a_label, VCD::Preset& a_preset)
        {
            auto index = static_cast<int>(VCD::ToUnderlying(a_preset));
            if (!GUI::Combo(a_label, &index, kPresetNames, static_cast<int>(std::size(kPresetNames)))) {
                return false;
            }

            a_preset = static_cast<VCD::Preset>(index);
            return true;
        }

        void SetCurrentHeight(VCD::CollisionData& a_data, const float& a_height)
        {
            const auto center = (a_data.capsule.point1.z + a_data.capsule.point2.z) * 0.5F;
            const auto halfHeight = a_height * 0.5F;
            a_data.capsule.point1.z = center + halfHeight;
            a_data.capsule.point2.z = center - halfHeight;
            a_data.RecalculateHeight();
        }

        void OpenPresetEditor(const VCD::Preset& a_preset)
        {
            auto& manager = VCD::Manager::GetSingleton();
            auto* presetConfig = manager.GetPresetConfig(a_preset);
            if (!presetConfig) {
                return;
            }

            auto& editor = GetPresetEditorState();
            editor.open = true;
            editor.preset = a_preset;
            editor.defaults = presetConfig->data;
            editor.current = presetConfig->data;
        }

        void RenderStateRow(const char* a_label, VCD::Preset& a_preset)
        {
            GUI::TableNextRow();
            GUI::TableNextColumn();
            GUI::Text("%s", a_label);
            GUI::TableNextColumn();
            GUI::SetNextItemWidth(180.0F);
            if (PresetCombo((std::string("##") + a_label).c_str(), a_preset)) {
                if (auto* player = RE::PlayerCharacter::GetSingleton()) {
                    Dynamics::ApplyPreset(player, a_preset, "preview");
                }
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

            const auto title = std::string("Edit Preset: ") + Dynamics::PresetName(editor.preset);
            constexpr auto windowSize = GUI::ImVec2{ 520.0F, 320.0F };
            GUI::SetNextWindowSize(windowSize, GUI::ImGuiCond_Appearing);
            if (const auto* io = GUI::GetIO()) {
                GUI::SetNextWindowPos(GUI::ImVec2{ io->DisplaySize.x * 0.5F, io->DisplaySize.y * 0.5F }, GUI::ImGuiCond_Appearing, GUI::ImVec2{ 0.5F, 0.5F });
            }

            if (!GUI::Begin(title.c_str(), &editor.open, 0)) {
                GUI::End();
                return;
            }

            GUI::PushItemWidth(260.0F);

            auto radius = editor.current.capsule.radius;
            if (GUI::SliderFloat("Radius", &radius, 0.0F, 0.8F)) {
                editor.current.capsule.radius = radius;
            }

            auto height = editor.current.capsule.height;
            if (GUI::SliderFloat("Height", &height, 0.1F, 1.5F)) {
                SetCurrentHeight(editor.current, height);
            }

            auto forward = editor.current.bump.translation.y;
            if (GUI::SliderFloat("Forward Offset", &forward, -50.0F, 50.0F)) {
                editor.current.bump.translation.y = forward;
            }

            auto side = editor.current.bump.translation.x;
            if (GUI::SliderFloat("Side Offset", &side, -50.0F, 50.0F)) {
                editor.current.bump.translation.x = side;
            }

            GUI::PopItemWidth();

            const bool dirty = !editor.current.IsSame(editor.defaults);
            if (CTAButton("Apply", dirty)) {
                presetConfig->data = editor.current;
                presetConfig->data.RecalculateHeight();
                Settings::MarkPresetEdited(editor.preset, presetConfig->data);
                editor.defaults = presetConfig->data;
                editor.current = presetConfig->data;

                if (auto* player = RE::PlayerCharacter::GetSingleton()) {
                    logger::info("Preset editor apply result: {}", manager.SetPreset(player, editor.preset));
                }
            }
            Tooltip("Apply this preset edit in-game without saving it to disk.");

            GUI::SameLine();

            const auto* defaultPresetConfig = manager.GetDefaultPresetConfig(editor.preset);
            const bool differsFromDefault = defaultPresetConfig && !editor.current.IsSame(defaultPresetConfig->data);
            if (CTAButton("Default", differsFromDefault)) {
                if (defaultPresetConfig) {
                    presetConfig->data = defaultPresetConfig->data;
                    Settings::ClearPresetEdited(editor.preset);
                    editor.defaults = presetConfig->data;
                    editor.current = presetConfig->data;

                    if (const auto* player = RE::PlayerCharacter::GetSingleton()) {
                        logger::info("Preset editor default result: {}", manager.SetPreset(player, editor.preset));
                    }
                }
            }
            Tooltip("Restore this preset from the preset JSON defaults without saving it to disk.");

            GUI::End();
        }
    }

    void Register()
    {
        if (!SKSEMenuFramework::IsInstalled()) {
            return;
        }

        SKSEMenuFramework::SetSection("Variadic Collisions");
        SKSEMenuFramework::AddSectionItem("Dynamics", RenderDynamicsMenu);
        SKSEMenuFramework::AddSectionItem("Debug", RenderDebugMenu);
    }

    void __stdcall RenderDynamicsMenu()
    {
        auto& config = Dynamics::GetConfig();
        auto& settings = Settings::GetSettings();

        GUI::Checkbox("Draw Bump Collision", &settings.drawCollision);
        GUI::Separator();

        if (GUI::BeginTable("DynamicsPresetTable", 2)) {
            GUI::TableSetupColumn("State", GUI::ImGuiTableColumnFlags_WidthFixed, 120.0F);
            GUI::TableSetupColumn("Preset", GUI::ImGuiTableColumnFlags_WidthStretch);

            DynamicsPanel::RenderStateRow("Outdoor", config.outdoor);
            DynamicsPanel::RenderStateRow("Indoor", config.indoor);
            DynamicsPanel::RenderStateRow("Combat", config.combat);
            DynamicsPanel::RenderStateRow("Neutral", config.neutral);

            GUI::EndTable();
        }

        GUI::Separator();

        if (CTAButton("Save Settings", Settings::IsDirty())) {
            Settings::Save();
        }
        Tooltip("Save draw collision, state mappings, and edited presets to disk.");

        GUI::SameLine();

        const Settings::VCDSettings defaults{};
        const bool changed = !Settings::SettingsEqual(Settings::GetSettings(), defaults);

        if (CTAButton("Default", changed)) {
            Settings::GetSettings() = defaults;
            Settings::ApplySettings(Settings::GetSettings());
        }
        Tooltip("Reset draw collision, state mappings, and preset overrides to defaults.");

        DynamicsPanel::RenderPresetEditor();
    }

    void __stdcall RenderDebugMenu()
    {
        using namespace VCD;

        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return;
        }

        if (GUI::Button("0.Vanilla Like"))
        {
            auto& manager = VCD::Manager::GetSingleton();
            logger::info("Debug menu SetPreset result: {}", manager.SetPreset(player, Preset::kVanillaLike));
        }

        if (GUI::Button("1.Personal Space"))
        {
            auto& manager = VCD::Manager::GetSingleton();
            logger::info("Debug menu SetPreset result: {}", manager.SetPreset(player, Preset::kPersonalSpace));
        }

        if (GUI::Button("2.Compact"))
        {
            auto& manager = VCD::Manager::GetSingleton();
            logger::info("Debug menu SetPreset result: {}", manager.SetPreset(player, Preset::kCompact));
        }

        if (GUI::Button("3.Bulky"))
        {
            auto& manager = VCD::Manager::GetSingleton();
            logger::info("Debug menu SetPreset result: {}", manager.SetPreset(player, Preset::kBulky));
        }
    }
}
