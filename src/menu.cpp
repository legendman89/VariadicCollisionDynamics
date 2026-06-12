#include "menu.hpp"
#include "button.hpp"
#include "dynamics.hpp"
#include "logger.hpp"
#include "manager.hpp"
#include "preset.hpp"
#include "settings.hpp"

#include <iterator>
#include <string>

namespace UI {

    namespace {
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

        bool SamePresetData(const VCD::CollisionData& a_left, const VCD::CollisionData& a_right)
        {
            return a_left.bump.translation.x == a_right.bump.translation.x &&
                a_left.bump.translation.y == a_right.bump.translation.y &&
                a_left.bump.translation.z == a_right.bump.translation.z &&
                a_left.bump.scale == a_right.bump.scale &&
                a_left.capsule.radius == a_right.capsule.radius &&
                a_left.capsule.height == a_right.capsule.height &&
                a_left.capsule.point1.x == a_right.capsule.point1.x &&
                a_left.capsule.point1.y == a_right.capsule.point1.y &&
                a_left.capsule.point1.z == a_right.capsule.point1.z &&
                a_left.capsule.point2.x == a_right.capsule.point2.x &&
                a_left.capsule.point2.y == a_right.capsule.point2.y &&
                a_left.capsule.point2.z == a_right.capsule.point2.z;
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
            PresetCombo(a_label, a_preset);
            GUI::TableNextColumn();
            if (GUI::Button((std::string("Edit##") + a_label).c_str())) {
                OpenPresetEditor(a_preset);
            }
        }

        void ApplyCurrentState()
        {
            auto* player = RE::PlayerCharacter::GetSingleton();
            if (!player) {
                return;
            }

            if (player->IsInCombat()) {
                Dynamics::ApplyPreset(player, Dynamics::GetConfig().combat, "combat");
            }
            else {
                Dynamics::ApplyEnvironmentPreset(player);
            }
        }

        VCD::Preset GetCurrentStatePreset(const RE::PlayerCharacter* a_player)
        {
            if (!a_player) {
                return Dynamics::GetConfig().neutral;
            }

            if (a_player->IsInCombat()) {
                return Dynamics::GetConfig().combat;
            }

            return Dynamics::GetCellPreset(a_player->GetParentCell());
        }

        bool CanApplyCurrentState()
        {
            auto* player = RE::PlayerCharacter::GetSingleton();
            if (!player || !VCD::Manager::GetSingleton().AreAllPresetsLoaded()) {
                return false;
            }

            const auto& state = Dynamics::GetPresetState();
            return !state.applied || state.current != GetCurrentStatePreset(player);
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
            GUI::SetNextWindowSize(GUI::ImVec2{ 520.0F, 320.0F }, GUI::ImGuiCond_Appearing);
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

            const bool dirty = !SamePresetData(editor.current, editor.defaults);
            if (CTAButton("Apply Now", dirty)) {
                presetConfig->data = editor.current;
                presetConfig->data.RecalculateHeight();
                Settings::MarkPresetEdited(editor.preset, presetConfig->data);
                Settings::Save();
                editor.defaults = presetConfig->data;
                editor.current = presetConfig->data;

                if (auto* player = RE::PlayerCharacter::GetSingleton()) {
                    logger::info("Preset editor apply result: {}", manager.SetPreset(player, editor.preset));
                }
            }

            GUI::SameLine();

            const auto* defaultPresetConfig = manager.GetDefaultPresetConfig(editor.preset);
            const bool differsFromDefault = defaultPresetConfig && !SamePresetData(editor.current, defaultPresetConfig->data);
            if (CTAButton("Default", differsFromDefault)) {
                Settings::ClearPresetEdited(editor.preset);

                if (auto* restoredPresetConfig = manager.GetPresetConfig(editor.preset)) {
                    editor.defaults = restoredPresetConfig->data;
                    editor.current = restoredPresetConfig->data;
                    Settings::Save();

                    if (auto* player = RE::PlayerCharacter::GetSingleton()) {
                        logger::info("Preset editor default result: {}", manager.SetPreset(player, editor.preset));
                    }
                }
            }

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

        GUI::Checkbox("Draw Player Collision", &settings.drawCollision);
        GUI::Separator();

        if (GUI::BeginTable("DynamicsPresetTable", 3, GUI::ImGuiTableFlags_BordersInnerV | GUI::ImGuiTableFlags_RowBg)) {
            GUI::TableSetupColumn("State");
            GUI::TableSetupColumn("Preset");
            GUI::TableSetupColumn("");
            GUI::TableHeadersRow();

            RenderStateRow("Outdoor", config.outdoor);
            RenderStateRow("Indoor", config.indoor);
            RenderStateRow("Combat", config.combat);
            RenderStateRow("Neutral", config.neutral);

            GUI::EndTable();
        }

        GUI::Separator();

        if (CTAButton("Save Settings", Settings::IsDirty())) {
            Settings::Save();
        }

        GUI::SameLine();

        if (CTAButton("Apply Now", CanApplyCurrentState())) {
            ApplyCurrentState();
        }

        GUI::SameLine();

        const Settings::VCDSettings defaults{};
        const bool changed = !Settings::SettingsEqual(Settings::GetSettings(), defaults);

        if (CTAButton("Default", changed)) {
            Settings::GetSettings() = defaults;
            Settings::ApplySettings(Settings::GetSettings());
        }

        RenderPresetEditor();
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
