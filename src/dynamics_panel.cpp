#include "dynamics_panel.hpp"
#include "config.hpp"
#include "tools_panel.hpp"
#include "draw.hpp"
#include "translate.hpp"

#include <algorithm>
#include <cstring>
#include <format>
#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>

namespace UI {

    std::vector<NPCActorOption> GetNearbyNPCActorOptions()
    {
        auto& drawState = DebugAPI_IMPL::Draw::GetNearbyActorDrawState();
        auto& npcState = Dynamics::GetNPCDynamicsState();

        std::vector<NPCActorOption> options{};
        std::unordered_map<std::string, int> nameCounts{};
        std::unordered_map<RE::FormID, bool> seen{};

        auto addNPCInfo = [&](const RE::ActorHandle& a_handle) {
            auto actorPtr = a_handle.get();
            auto* actor = actorPtr.get();
            if (!actor || seen[actor->GetFormID()]) {
                return;
            }

            seen[actor->GetFormID()] = true;
            const auto name = GetActorName(actor);
            nameCounts[name]++;
            options.push_back({ a_handle, name, name, actor->GetFormID() });
        };

        for (auto& handle : npcState.nearbyActors) {
            addNPCInfo(handle);
        }

        for (auto& handle : drawState.handles) {
            auto actorPtr = handle.get();
            auto* actor = actorPtr.get();
            if (!actor || (!actor->IsGuard() && !actor->HasKeywordString("ActorTypeNPC"))) {
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
            [](const NPCActorOption& a_left, const NPCActorOption& a_right) {
                return a_left.label < a_right.label;
            }
        );

        return options;
    }

    bool PresetCombo(const char* a_label, VCD::Preset& a_preset)
    {
        const auto& manager = VCD::Manager::GetSingleton();
        const auto& presetConfigs = manager.GetPresetConfigs();
        const auto* current = manager.GetPresetConfig(a_preset);
        bool changed = false;

        SolidBackground(GUI::ImGuiCol_PopupBg);
        if (GUI::BeginCombo(a_label, current ? current->name.c_str() : Trans::Tr("Dynamics.Label.Vanilla").c_str())) {
            for (size_t i = 0; i < presetConfigs.size(); ++i) {
                if (i == VCD::kBuiltInPresetCount) {
                    GUI::Separator();
                }

                const auto& presetConfig = presetConfigs[i];
                const bool selected = presetConfig.preset == a_preset;
                if (GUI::Selectable(presetConfig.name.c_str(), selected)) {
                    a_preset = presetConfig.preset;
                    changed = true;
                }
            }
            GUI::EndCombo();
        }
        GUI::PopStyleColor();
        return changed;
    }

    void OpenPresetEditor(const VCD::Preset& a_preset)
    {
        if (GetCreatePresetEditorState().open) {
            CloseCreatePresetEditor();
        }

        auto& manager = VCD::Manager::GetSingleton();
        auto* presetConfig = manager.GetPresetConfig(a_preset);
        const auto* defaultData = GetDefaultPresetData(a_preset);
        if (!presetConfig || !defaultData) {
            return;
        }

        auto& editor = GetPresetEditorState();
        if (editor.open) {
            StopPresetEditorPreview();
        }

        editor.open = true;
        editor.preview = Dynamics::IsPresetPreviewed(a_preset);
        editor.npcPreview = false;
        editor.npcGlobal = false;
        editor.autoEnabledDraw = false;
        editor.previousDrawCollision = false;
        editor.previousDrawNearbyActors = false;
        editor.previewActor = {};
        editor.preset = a_preset;
        editor.defaults = *defaultData;
        editor.current = presetConfig->data;

        if (editor.preview) {
            if (const auto* player = RE::PlayerCharacter::GetSingleton()) {
                Dynamics::StartPresetPreview(player, a_preset);
            }
        }
    }

    void OpenNPCGlobalPresetEditor(const VCD::Preset& a_preset)
    {
        if (GetCreatePresetEditorState().open) {
            CloseCreatePresetEditor();
        }

        if (!Settings::GetSettings().enableNPCDynamics) {
            return;
        }

        const auto* defaultData = GetDefaultPresetData(a_preset);
        if (!defaultData) {
            return;
        }

        auto& editor = GetPresetEditorState();
        if (editor.open) {
            StopPresetEditorPreview();
        }

        editor.open = true;
        editor.preview = false;
        editor.npcPreview = false;
        editor.npcGlobal = true;
        editor.autoEnabledDraw = false;
        editor.previousDrawCollision = false;
        editor.previousDrawNearbyActors = false;
        editor.previewActor = {};
        editor.preset = a_preset;
        editor.defaults = *defaultData;
        if (const auto* npcOverride = Settings::GetNPCPresetOverride(a_preset)) {
            editor.current = *npcOverride;
        }
        else {
            editor.current = *defaultData;
        }
    }

    void OpenNPCPresetEditor(const VCD::Preset& a_preset, RE::Actor* a_actor)
    {
        if (GetCreatePresetEditorState().open) {
            CloseCreatePresetEditor();
        }

        if (!Settings::GetSettings().enableNPCDynamics) {
            return;
        }

        const auto* defaultData = GetDefaultNPCPresetData(a_preset);
        if (!defaultData || !a_actor) {
            return;
        }

        auto& editor = GetPresetEditorState();
        if (editor.open) {
            StopPresetEditorPreview();
        }

        editor.open = true;
        editor.preview = true;
        editor.npcPreview = true;
        editor.npcGlobal = false;
        editor.autoEnabledDraw = false;
        editor.previousDrawCollision = false;
        editor.previousDrawNearbyActors = false;
        editor.previewActor = a_actor->CreateRefHandle();
        editor.preset = a_preset;
        editor.defaults = *defaultData;
        if (const auto* actorOverride = Settings::GetNPCActorPresetOverride(a_actor->GetFormID(), a_preset)) {
            editor.current = *actorOverride;
        }
        else {
            editor.current = *defaultData;
        }

        BeginAutoDraw();
        Dynamics::StartNPCPresetPreview(a_actor, a_preset);
    }

    bool RefreshPresetEditor()
    {
        auto& editor = GetPresetEditorState();
        if (!editor.open) {
            return false;
        }

        auto& manager = VCD::Manager::GetSingleton();
        const auto* defaultData = GetDefaultPresetData(editor.preset);
        auto* presetConfig = manager.GetPresetConfig(editor.preset);
        if (!defaultData || !presetConfig) {
            editor.open = false;
            return false;
        }

        editor.defaults = *defaultData;
        if (editor.npcGlobal) {
            if (const auto* npcOverride = Settings::GetNPCPresetOverride(editor.preset)) {
                editor.current = *npcOverride;
            }
            else {
                editor.current = *defaultData;
            }
            return true;
        }

        if (editor.npcPreview) {
            if (const auto* defaultNPCData = GetDefaultNPCPresetData(editor.preset)) {
                editor.defaults = *defaultNPCData;
            }

            auto actorPtr = editor.previewActor.get();
            auto* actor = actorPtr.get();
            if (actor) {
                if (const auto* actorOverride = Settings::GetNPCActorPresetOverride(actor->GetFormID(), editor.preset)) {
                    editor.current = *actorOverride;
                    return true;
                }
            }
        }

        editor.current = editor.npcPreview ? editor.defaults : presetConfig->data;
        return true;
    }

    void BeginAutoDraw()
    {
        auto& editor = GetPresetEditorState();
        auto& settings = Settings::GetSettings();
        if (!settings.autoDrawPreview || editor.autoEnabledDraw) {
            return;
        }

        editor.previousDrawCollision = settings.drawCollision;
        editor.previousDrawNearbyActors = settings.drawNearbyActors;
        editor.autoEnabledDraw = true;
        if (editor.npcPreview || editor.npcGlobal) {
            settings.drawNearbyActors = true;
        }
        else {
            settings.drawCollision = true;
        }
    }

    void EndAutoDraw()
    {
        auto& editor = GetPresetEditorState();
        if (!editor.autoEnabledDraw) {
            return;
        }

        Settings::GetSettings().drawCollision = editor.previousDrawCollision;
        Settings::GetSettings().drawNearbyActors = editor.previousDrawNearbyActors;
        editor.autoEnabledDraw = false;
        ClearDrawLines();
    }

    void StopPresetEditorPreview()
    {
        auto& editor = GetPresetEditorState();
        if (editor.npcPreview || editor.npcGlobal) {
            Dynamics::StopNPCPresetPreview();
        }
        else if (editor.preview) {
            if (const auto* player = RE::PlayerCharacter::GetSingleton()) {
                Dynamics::RestorePresetPreview(player);
            }
        }

        EndAutoDraw();
        editor.preview = false;
        editor.npcPreview = false;
        editor.npcGlobal = false;
    }

    void ClosePresetEditor()
    {
        auto& editor = GetPresetEditorState();
        if (editor.npcPreview || editor.npcGlobal) {
            Dynamics::StopNPCPresetPreview();
        }
        else if (editor.preview) {
            Dynamics::SchedulePreviewRestore(Settings::GetSettings().previewRestoreDelay);
        }

        EndAutoDraw();
        editor.preview = false;
        editor.npcPreview = false;
        editor.npcGlobal = false;
    }

    void UpdateEditedPreset()
    {
        auto& editor = GetPresetEditorState();
        if ((editor.npcPreview || editor.npcGlobal) && !Settings::GetSettings().enableNPCDynamics) {
            return;
        }

        auto& manager = VCD::Manager::GetSingleton();
        auto* presetConfig = manager.GetPresetConfig(editor.preset);
        const auto* defaultPresetConfig = manager.GetDefaultPresetConfig(editor.preset);
        if (!presetConfig || !defaultPresetConfig) {
            return;
        }

		// Ordered by most specific to least specific preview type.
		// NPC Actor Preview -> NPC Global Preview. Player is decoupled.
        if (editor.npcPreview) {
            auto actorPtr = editor.previewActor.get();
            auto* actor = actorPtr.get();
            if (actor) {
                BeginAutoDraw();
                editor.current.RecalculateHeight();
                Settings::MarkNPCActorPresetEdited(actor->GetFormID(), GetActorName(actor), editor.preset, editor.current);
                manager.SetCollisionData(actor, editor.current, editor.preset, VCD::PresetName(editor.preset), PoseFixes::NPCPose(actor), false);
            }
        }
        else if (editor.npcGlobal) {
            editor.current.RecalculateHeight();
            Settings::MarkNPCPresetEdited(editor.preset, editor.current);
            Dynamics::GetNPCDynamicsState().actors.clear();
            if (auto* actor = GetSelectedNPCActorPtr()) {
                BeginAutoDraw();
                editor.preview = true;
                editor.previewActor = actor->CreateRefHandle();
                Dynamics::StartNPCPresetPreview(actor, editor.preset);
            }
        }
        else if (const auto* player = RE::PlayerCharacter::GetSingleton()) {
            presetConfig->data = editor.current;
            presetConfig->data.RecalculateHeight();
            Settings::MarkPresetEdited(editor.preset, presetConfig->data);

            if (Dynamics::IsPresetCurrent(editor.preset)) {
                BeginAutoDraw();
                manager.SetPreset(player, editor.preset, PoseFixes::PlayerPose(player), true);
            }
            else if (editor.preview) {
                BeginAutoDraw();
                Dynamics::StartPresetPreview(player, editor.preset);
            }
        }
    }

    void RestoreCreatePresetPreview()
    {
        auto& editor = GetCreatePresetEditorState();
        if (!editor.preview) {
            return;
        }

        if (const auto* player = RE::PlayerCharacter::GetSingleton()) {
            const auto& state = Dynamics::GetPresetState();
            if (state.applied) {
                VCD::Manager::GetSingleton().SetPreset(player, state.current, PoseFixes::PlayerPose(player), true);
            }
            else {
                Dynamics::ApplyEnvironmentPreset(player, true);
            }
        }

        editor.preview = false;
    }

    void ApplyCreatePresetPreview()
    {
        auto& editor = GetCreatePresetEditorState();
        if (!editor.preview) {
            return;
        }

        if (const auto* player = RE::PlayerCharacter::GetSingleton()) {
            editor.current.RecalculateHeight();
            VCD::Manager::GetSingleton().SetCollisionData(
                player,
                editor.current,
                VCD::Preset::kVanilla,
                "New Preset",
                PoseFixes::PlayerPose(player),
                false
            );
        }
    }

    void OpenCreatePresetEditor()
    {
        CloseDeletePresetEditor();
        auto& presetEditor = GetPresetEditorState();
        if (presetEditor.open) {
            ClosePresetEditor();
            presetEditor.open = false;
        }

        const auto* vanilla = VCD::Manager::GetSingleton().GetDefaultPresetConfig(VCD::Preset::kVanilla);
        if (!vanilla || !vanilla->loaded) {
            return;
        }

        auto& editor = GetCreatePresetEditorState();
        RestoreCreatePresetPreview();
        editor = {};
        editor.open = true;
        editor.defaults = vanilla->data;
        editor.current = vanilla->data;
    }

    void CloseCreatePresetEditor()
    {
        RestoreCreatePresetPreview();
        GetCreatePresetEditorState().open = false;
    }

    void OpenDeletePresetEditor()
    {
        if (GetCreatePresetEditorState().open) {
            CloseCreatePresetEditor();
        }

        auto& editor = GetDeletePresetEditorState();
        editor = {};
        for (const auto& config : VCD::Manager::GetSingleton().GetPresetConfigs()) {
            if (!config.builtIn) {
                editor.open = true;
                editor.preset = config.preset;
                return;
            }
        }
    }

    void CloseDeletePresetEditor()
    {
        GetDeletePresetEditorState() = {};
    }

    bool RenderCollisionSliders(VCD::CollisionData& a_current, const VCD::CollisionData& a_defaults)
    {
        bool changed = false;
        GUI::PushStyleColor(GUI::ImGuiCol_FrameBg, Color::kEditFrameBg);
        GUI::PushStyleColor(GUI::ImGuiCol_FrameBgHovered, Color::kEditFrameHover);
        GUI::PushStyleColor(GUI::ImGuiCol_FrameBgActive, Color::kEditFrameActive);
        GUI::PushItemWidth(260.0F);

        constexpr auto kOffsetLimit = 30.0F;
        constexpr auto kHeightLimit = 4.0F;
        const auto defaultTop = a_defaults.capsule.point1.z;
        const auto defaultBottom = a_defaults.capsule.point2.z;

        auto radius = a_current.capsule.radius;
        if (GUI::SliderFloat(Trans::Tr("Dynamics.Editor.Radius").c_str(), &radius, 0.05F, 1.0F)) {
            a_current.capsule.radius = radius;
            changed = true;
        }

        auto topOffset = a_current.capsule.point1.z - defaultTop;
        topOffset = std::clamp(topOffset, 0.0F, kHeightLimit);
        if (GUI::SliderFloat(Trans::Tr("Dynamics.Editor.TopOffset").c_str(), &topOffset, 0.0F, kHeightLimit)) {
            a_current.capsule.point1.z = defaultTop + topOffset;
            changed = true;
        }

        auto bottomOffset = a_current.capsule.point2.z - defaultBottom;
        bottomOffset = std::clamp(bottomOffset, -kHeightLimit, 0.0F);
        if (GUI::SliderFloat(Trans::Tr("Dynamics.Editor.BottomOffset").c_str(), &bottomOffset, -kHeightLimit, 0.0F)) {
            a_current.capsule.point2.z = defaultBottom + bottomOffset;
            changed = true;
        }

        auto forward = a_current.bump.translation.y;
        if (GUI::SliderFloat(Trans::Tr("Dynamics.Editor.ForwardOffset").c_str(), &forward, -kOffsetLimit, kOffsetLimit)) {
            a_current.bump.translation.y = forward;
            changed = true;
        }

        auto side = a_current.bump.translation.x;
        if (GUI::SliderFloat(Trans::Tr("Dynamics.Editor.SideOffset").c_str(), &side, -kOffsetLimit, kOffsetLimit)) {
            a_current.bump.translation.x = side;
            changed = true;
        }

        GUI::PopItemWidth();
        GUI::PopStyleColor(3);
        return changed;
    }

    void RenderStateRow(const char* a_label, VCD::Preset& a_preset, const bool& a_previewPlayer = true, const bool& a_showEdit = true, const bool& a_npcGlobal = false)
    {
        GUI::TableNextRow();
        GUI::TableNextColumn();
        GUI::Text("%s", a_label);
        GUI::TableNextColumn();
        GUI::SetNextItemWidth(180.0F);
        if (PresetCombo((std::string("##") + a_label).c_str(), a_preset) && a_previewPlayer) {
            PreviewPreset(a_preset);
        }
        if (a_showEdit) {
            GUI::SameLine();
            GUI::PushStyleColor(GUI::ImGuiCol_ButtonHovered, Color::kEditHover);
            GUI::PushStyleColor(GUI::ImGuiCol_ButtonActive, Color::kEditActive);
            if (GUI::Button((Trans::Tr("Dynamics.Editor.Button.Edit") + "##" + a_label).c_str())) {
                if (a_npcGlobal) {
                    OpenNPCGlobalPresetEditor(a_preset);
                }
                else {
                    OpenPresetEditor(a_preset);
                }
            }
            WrappedTooltip(a_npcGlobal
                ? Trans::Tr("Dynamics.Editor.Details.NPCGlobal").c_str()
                : Trans::Tr("Dynamics.Editor.Details.StateLocal").c_str());
            GUI::PopStyleColor(2);
        }
    }

    void RenderNPCActorSelector(VCD::Preset& a_selectedPreset)
    {
        auto options = GetNearbyNPCActorOptions();
        auto* selectedActor = GetSelectedNPCActorPtr();
        std::string preview = Trans::Tr("Dynamics.NPC.SelectActorPreview");

        if (!selectedActor && !options.empty()) {
            GetSelectedNPCActor() = options.front().handle;
            selectedActor = GetSelectedNPCActorPtr();
        }

        if (selectedActor) {
            preview = GetActorName(selectedActor);
            for (auto& option : options) {
                auto actorPtr = option.handle.get();
                if (actorPtr.get() == selectedActor) {
                    preview = option.label;
                    break;
                }
            }
        }

        GUI::SetNextItemWidth(260.0F);
        SolidBackground(GUI::ImGuiCol_PopupBg);

        if (GUI::BeginCombo(Trans::Tr("Dynamics.NPC.ActorComboLabel").c_str(), preview.c_str())) {
            for (auto& option : options) {
                auto actorPtr = option.handle.get();
                const auto selected = selectedActor && actorPtr.get() == selectedActor;

                if (GUI::Selectable(option.label.c_str(), selected)) {
                    GetSelectedNPCActor() = option.handle;
                }
            }
            GUI::EndCombo();
        }

        GUI::PopStyleColor(); // Pop Bg color

        Tooltip(Trans::Tr("Dynamics.NPC.ActorComboTooltip").c_str());

        GUI::SameLine();
        GUI::SetNextItemWidth(180.0F);

        PresetCombo(Trans::Tr("Dynamics.NPC.PreviewPresetLabel").c_str(), a_selectedPreset);

        GUI::SameLine();

        GUI::PushStyleColor(GUI::ImGuiCol_ButtonHovered, Color::kEditHover);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonActive, Color::kEditActive);

        if (GUI::Button(Trans::Tr("Dynamics.NPC.EditSelectedActorButton").c_str())) {
            OpenNPCPresetEditor(a_selectedPreset, GetSelectedNPCActorPtr());
        }

        Tooltip(Trans::Tr("Dynamics.NPC.EditSelectedActorTooltip").c_str());

        GUI::PopStyleColor(2);
    }

    void RenderCreateDeleteButtons()
    {
        constexpr unsigned kCirclePlusIcon = 0xF055;
        constexpr auto buttonWidth = 220.0F;
        constexpr auto buttonHeight = 50.0F;
        static const auto circlePlusText = FontAwesome::UnicodeToUtf8(kCirclePlusIcon);

        GUI::PushStyleVar(GUI::ImGuiStyleVar_FrameRounding, 5.0F);
        GUI::PushStyleVar(GUI::ImGuiStyleVar_FrameBorderSize, 1.0F);
        GUI::PushStyleColor(GUI::ImGuiCol_Button, UI::Color::kCreatePresetBG);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonHovered, UI::Color::kCreatePresetHover);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonActive, UI::Color::kCreatePresetActive);
        GUI::PushStyleColor(GUI::ImGuiCol_Text, UI::Color::kCreatePresetText);
        GUI::PushStyleColor(GUI::ImGuiCol_Border, UI::Color::kCreatePresetBorder);

        const bool createPreset =
            GUI::Button(
                (Trans::Tr("Dynamics.Preset.CreateButton") + "##CreateNewPreset").c_str(),
                GUI::ImVec2{ buttonWidth, buttonHeight }
            );

        WrappedTooltip(
            Trans::Tr("Dynamics.Preset.CreateTooltip").c_str()
        );

        GUI::ImVec2 buttonMin{};
        GUI::ImVec2 buttonMax{};
        GUI::ImVec2 nextItemPosition{};

        GUI::GetItemRectMin(&buttonMin);
        GUI::GetItemRectMax(&buttonMax);
        GUI::GetCursorScreenPos(&nextItemPosition);

        FontAwesome::PushSolid();

        GUI::ImVec2 iconSize{};
        GUI::CalcTextSize(&iconSize, circlePlusText.c_str(), nullptr, false, -1.0F);

        GUI::SetCursorScreenPos(
            GUI::ImVec2{
                buttonMin.x + 13.0F,
                buttonMin.y + ((buttonMax.y - buttonMin.y - iconSize.y) * 0.5F)
            }
        );

        GUI::TextUnformatted(circlePlusText.c_str());

        FontAwesome::Pop();

        GUI::SetCursorScreenPos(nextItemPosition);

        if (createPreset) {
            OpenCreatePresetEditor();
        }

        GUI::PopStyleColor(5);
        GUI::PopStyleVar(2);

        GUI::SameLine();

        constexpr unsigned kCircleMinusIcon = 0xF056;
        static const auto circleMinusText = FontAwesome::UnicodeToUtf8(kCircleMinusIcon);
        const auto& presetConfigs = VCD::Manager::GetSingleton().GetPresetConfigs();
        const auto hasCustomPresets = std::any_of(presetConfigs.begin(), presetConfigs.end(),
            [](const VCD::PresetConfig& a_config) {
                return !a_config.builtIn;
            }
        );

        GUI::BeginDisabled(!hasCustomPresets);
        GUI::PushStyleVar(GUI::ImGuiStyleVar_FrameRounding, 5.0F);
        GUI::PushStyleVar(GUI::ImGuiStyleVar_FrameBorderSize, 1.0F);
        GUI::PushStyleColor(GUI::ImGuiCol_Button, UI::Color::kDeletePresetBG);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonHovered, UI::Color::kDeletePresetHover);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonActive, UI::Color::kDeletePresetActive);
        GUI::PushStyleColor(GUI::ImGuiCol_Text, UI::Color::kDeletePresetText);
        GUI::PushStyleColor(GUI::ImGuiCol_Border, UI::Color::kDeletePresetBorder);

        const bool deletePreset = GUI::Button("        Delete Preset##DeletePreset", GUI::ImVec2{ buttonWidth, buttonHeight });
        WrappedTooltip(hasCustomPresets ? "Delete a custom preset." : "There are no custom presets to delete.");

        GUI::GetItemRectMin(&buttonMin);
        GUI::GetItemRectMax(&buttonMax);
        GUI::GetCursorScreenPos(&nextItemPosition);

        FontAwesome::PushSolid();
        GUI::CalcTextSize(&iconSize, circleMinusText.c_str(), nullptr, false, -1.0F);
        GUI::SetCursorScreenPos(GUI::ImVec2{ buttonMin.x + 13.0F, buttonMin.y + ((buttonMax.y - buttonMin.y - iconSize.y) * 0.5F) });
        GUI::TextUnformatted(circleMinusText.c_str());
        FontAwesome::Pop();
        GUI::SetCursorScreenPos(nextItemPosition);

        if (deletePreset) {
            OpenDeletePresetEditor();
        }

        GUI::PopStyleColor(5);
        GUI::PopStyleVar(2);
        GUI::EndDisabled();

        GUI::Spacing();
        GUI::Spacing();
        GUI::Separator();
    }

    void RenderPlayerDynamics()
    {
        auto& config = Dynamics::GetConfig();

        if (GUI::BeginTable("PlayerDynamicsPresetTable", 2)) {

            GUI::TableSetupColumn(Trans::Tr("Dynamics.PlayerDynamics.Column.State").c_str(), GUI::ImGuiTableColumnFlags_WidthFixed, 120.0F);
            GUI::TableSetupColumn(Trans::Tr("Dynamics.PlayerDynamics.Column.Preset").c_str(), GUI::ImGuiTableColumnFlags_WidthStretch);

            RenderStateRow(Trans::Tr("Dynamics.PlayerDynamics.State.Outdoor").c_str(), config.outdoor);
            RenderStateRow(Trans::Tr("Dynamics.PlayerDynamics.State.Indoor").c_str(), config.indoor);
            RenderStateRow(Trans::Tr("Dynamics.PlayerDynamics.State.Combat").c_str(), config.combat);
            RenderStateRow(Trans::Tr("Dynamics.PlayerDynamics.State.Werewolf").c_str(), config.werewolf);
            RenderStateRow(Trans::Tr("Dynamics.PlayerDynamics.State.VampireLord").c_str(), config.vampireLord);
            RenderStateRow(Trans::Tr("Dynamics.PlayerDynamics.State.Neutral").c_str(), config.neutral);

            GUI::EndTable();
        }
    }

    void RenderNPCDynamics()
    {
        auto& config = Dynamics::GetConfig();
        auto& settings = Settings::GetSettings();

        if (GUI::Checkbox(Trans::Tr("Dynamics.NPC.EnableCheckbox").c_str(), &settings.enableNPCDynamics)
            && !settings.enableNPCDynamics)
        {
            Dynamics::RestoreNPCsToVanilla();

            auto& editor = GetPresetEditorState();
            if (editor.npcPreview || editor.npcGlobal) {
                ClosePresetEditor();
                editor.open = false;
            }
        }

        GUI::BeginDisabled(!settings.enableNPCDynamics);

        if (GUI::BeginTable("NPCDynamicsPresetTable", 2)) {

            GUI::TableSetupColumn(Trans::Tr("Dynamics.NPC.Column.State").c_str(), GUI::ImGuiTableColumnFlags_WidthFixed, 120.0F);
            GUI::TableSetupColumn(Trans::Tr("Dynamics.NPC.Column.Preset").c_str(), GUI::ImGuiTableColumnFlags_WidthStretch);

            RenderStateRow(Trans::Tr("Dynamics.NPC.State.NPCNeutral").c_str(), config.npcNeutral, false, true, true);
            RenderStateRow(Trans::Tr("Dynamics.NPC.State.NPCCombat").c_str(), config.npcCombat, false, true, true);
            RenderStateRow(Trans::Tr("Dynamics.NPC.State.GuardNeutral").c_str(), config.guardNeutral, false, true, true);
            RenderStateRow(Trans::Tr("Dynamics.NPC.State.GuardCombat").c_str(), config.guardCombat, false, true, true);

            GUI::EndTable();
        }

        GUI::Spacing();
        RenderNPCActorSelector(GetSelectedNPCPreset());

        GUI::EndDisabled();
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

        if ((editor.npcPreview || editor.npcGlobal) && !Settings::GetSettings().enableNPCDynamics) {
            ClosePresetEditor();
            editor.open = false;
            return;
        }

        const auto actorName =
            editor.npcPreview
            ? GetActorName(GetSelectedNPCActorPtr())
            : (editor.npcGlobal ? std::string("NPC") : std::string("Player"));

        const auto title =
            Trans::Tr("Dynamics.Editor.WindowTitlePrefix") + " " +
            actorName +
            " " +
            Trans::Tr("Dynamics.Editor.WindowTitlePreset") + " " +
            VCD::PresetName(editor.preset);

        constexpr auto windowSize = GUI::ImVec2{ 480.0F, 380.0F };
        GUI::SetNextWindowSize(windowSize, GUI::ImGuiCond_Appearing);

        if (const auto* io = GUI::GetIO()) {
            GUI::SetNextWindowPos(
                GUI::ImVec2{ io->DisplaySize.x * 0.5F, io->DisplaySize.y * 0.5F },
                GUI::ImGuiCond_Appearing,
                GUI::ImVec2{ 0.5F, 0.5F }
            );
        }

        const auto wasOpen = editor.open;

        GUI::PushStyleColor(GUI::ImGuiCol_WindowBg, Color::kEditWindowBg);

        const bool opened = GUI::Begin(title.c_str(), &editor.open, 0);
        if (!opened) {
            GUI::End();

            if (wasOpen && !editor.open) {
                ClosePresetEditor();
            }

            GUI::PopStyleColor();
            return;
        }

        const auto isCurrentPreset = Dynamics::IsPresetCurrent(editor.preset);

        if (!isCurrentPreset) {
            const std::string previewLabel =
                Trans::Tr("Dynamics.Editor.PreviewPrefix") + " " +
                actorName +
                " " +
                Trans::Tr("Dynamics.Editor.PresetLabel");

            if (!editor.npcPreview && !editor.npcGlobal &&
                GUI::Checkbox(previewLabel.c_str(), &editor.preview))
            {
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

            WrappedTooltip(
                Trans::Tr("Dynamics.Editor.PreviewTooltip").c_str()
            );
        }

        if (RenderCollisionSliders(editor.current, editor.defaults)) {
            UpdateEditedPreset();
        }

        GUI::Spacing();

        const bool differsFromDefault = !editor.current.IsSame(editor.defaults);

        if (CTAButton(Trans::Tr("Dynamics.Editor.ResetButton").c_str(), differsFromDefault)) {
            if (editor.npcPreview) {
                auto actorPtr = editor.previewActor.get();
                auto* actor = actorPtr.get();

                if (actor) {
                    BeginAutoDraw();

                    Settings::ClearNPCActorPresetEdited(actor->GetFormID(), editor.preset);
                    editor.current = editor.defaults;

                    manager.SetCollisionData(
                        actor,
                        editor.current,
                        editor.preset,
                        VCD::PresetName(editor.preset),
                        PoseFixes::NPCPose(actor),
                        false
                    );
                }
            }
            else if (editor.npcGlobal) {
                Settings::ClearNPCPresetEdited(editor.preset);

                editor.current = editor.defaults;
                Dynamics::GetNPCDynamicsState().actors.clear();

                if (auto* actor = GetSelectedNPCActorPtr()) {
                    BeginAutoDraw();

                    editor.preview = true;
                    editor.previewActor = actor->CreateRefHandle();

                    Dynamics::StartNPCPresetPreview(actor, editor.preset);
                }
            }
            else if (const auto* player = RE::PlayerCharacter::GetSingleton()) {
                presetConfig->data = editor.defaults;

                Settings::ClearPresetEdited(editor.preset);
                editor.current = editor.defaults;

                if (Dynamics::IsPresetCurrent(editor.preset)) {
                    BeginAutoDraw();
                    manager.SetPreset(player, editor.preset, PoseFixes::PlayerPose(player), true);
                }
                else if (editor.preview) {
                    BeginAutoDraw();
                    Dynamics::StartPresetPreview(player, editor.preset);
                }
            }
        }

        WrappedTooltip(
            Trans::Tr("Dynamics.Editor.ResetTooltip").c_str()
        );

        GUI::End();

        if (wasOpen && !editor.open) {
            ClosePresetEditor();
        }

        GUI::PopStyleColor();
    }

    void RenderDeletePresetEditor()
    {
        auto& editor = GetDeletePresetEditorState();
        if (!editor.open) {
            return;
        }

        const auto& manager = VCD::Manager::GetSingleton();
        const auto* selected = manager.GetPresetConfig(editor.preset);
        if (!selected || selected->builtIn) {
            CloseDeletePresetEditor();
            return;
        }

        constexpr auto windowSize = GUI::ImVec2{ 420.0F, 180.0F };
        GUI::SetNextWindowSize(windowSize, GUI::ImGuiCond_Appearing);
        if (const auto* io = GUI::GetIO()) {
            GUI::SetNextWindowPos(GUI::ImVec2{ io->DisplaySize.x * 0.5F, io->DisplaySize.y * 0.5F }, GUI::ImGuiCond_Appearing, GUI::ImVec2{ 0.5F, 0.5F });
        }

        const auto wasOpen = editor.open;
        GUI::PushStyleColor(GUI::ImGuiCol_WindowBg, Color::kEditWindowBg);
        if (!GUI::Begin("Delete Preset", &editor.open, 0)) {
            GUI::End();
            if (wasOpen && !editor.open) {
                CloseDeletePresetEditor();
            }
            GUI::PopStyleColor();
            return;
        }

        GUI::SetNextItemWidth(260.0F);
        SolidBackground(GUI::ImGuiCol_PopupBg);
        if (GUI::BeginCombo("Preset", selected->name.c_str())) {
            for (const auto& config : manager.GetPresetConfigs()) {
                if (config.builtIn) {
                    continue;
                }
                const auto isSelected = config.preset == editor.preset;
                if (GUI::Selectable(config.name.c_str(), isSelected)) {
                    editor.preset = config.preset;
                    editor.error.clear();
                }
            }
            GUI::EndCombo();
        }
        GUI::PopStyleColor();

        GUI::TextWrapped("This permanently deletes the selected preset file.");
        if (CTAButton("Delete", true)) {
            const auto deletedPreset = editor.preset;
            std::string key{};
            if (VCD::Manager::GetSingleton().DeletePreset(deletedPreset, key, editor.error)) {
                auto& presetEditor = GetPresetEditorState();
                if (presetEditor.open) {
                    EndAutoDraw();
                    presetEditor = {};
                }
                Settings::RemapDeletedPreset(deletedPreset, key);
                Dynamics::RemapDeletedPreset(deletedPreset);
                auto& selectedNPCPreset = GetSelectedNPCPreset();
                VCD::RemapPresetAfterDeletion(selectedNPCPreset, deletedPreset);
                CloseDeletePresetEditor();
            }
        }
        GUI::SameLine();
        if (GUI::Button("Cancel")) {
            CloseDeletePresetEditor();
        }

        if (!editor.error.empty()) {
            GUI::TextWrapped("%s", editor.error.c_str());
        }

        GUI::End();
        if (wasOpen && !editor.open) {
            CloseDeletePresetEditor();
        }
        GUI::PopStyleColor();
    }

    void RenderCreatePresetEditor()
    {
        auto& editor = GetCreatePresetEditorState();
        if (!editor.open) {
            return;
        }

        constexpr auto windowSize = GUI::ImVec2{ 480.0F, 440.0F };
        GUI::SetNextWindowSize(windowSize, GUI::ImGuiCond_Appearing);

        if (const auto* io = GUI::GetIO()) {
            GUI::SetNextWindowPos(
                GUI::ImVec2{ io->DisplaySize.x * 0.5F, io->DisplaySize.y * 0.5F },
                GUI::ImGuiCond_Appearing,
                GUI::ImVec2{ 0.5F, 0.5F }
            );
        }

        const auto wasOpen = editor.open;

        GUI::PushStyleColor(GUI::ImGuiCol_WindowBg, Color::kEditWindowBg);

        const bool opened =
            GUI::Begin(Trans::Tr("Dynamics.CreatePreset.WindowTitle").c_str(), &editor.open, 0);

        if (!opened) {
            GUI::End();

            if (wasOpen && !editor.open) {
                CloseCreatePresetEditor();
            }

            GUI::PopStyleColor();
            return;
        }

        GUI::SetNextItemWidth(260.0F);

        if (GUI::InputText(Trans::Tr("Dynamics.CreatePreset.NameLabel").c_str(),
            editor.name.data(),
            editor.name.size()))
        {
            editor.error.clear();
        }

        if (GUI::Checkbox(Trans::Tr("Dynamics.CreatePreset.PreviewCheckbox").c_str(), &editor.preview)) {
            if (editor.preview) {
                ApplyCreatePresetPreview();
            }
            else {
                editor.preview = true;
                RestoreCreatePresetPreview();
            }
        }

        if (RenderCollisionSliders(editor.current, editor.defaults)) {
            ApplyCreatePresetPreview();
        }

        GUI::Spacing();

        const bool differsFromDefault = !editor.current.IsSame(editor.defaults);

        if (CTAButton(Trans::Tr("Dynamics.CreatePreset.DefaultButton").c_str(), differsFromDefault)) {
            editor.current = editor.defaults;
            ApplyCreatePresetPreview();
        }

        GUI::SameLine();

        const std::string name = editor.name.data();
        const auto key = VCD::MakePresetKey(name);
        const auto duplicate = !key.empty() && VCD::Manager::GetSingleton().GetPresetConfig(key);

        const bool canSave = !key.empty() && !duplicate;

        std::string validationError{};

        if (key.empty() && !name.empty()) {
            validationError = Trans::Tr("Dynamics.CreatePreset.Error.InvalidName");
        }
        else if (duplicate) {
            validationError = Trans::Tr("Dynamics.CreatePreset.Error.Duplicate");
        }

        if (CTAButton(Trans::Tr("Dynamics.CreatePreset.SaveButton").c_str(), canSave)) {
            VCD::Preset preset{};

            if (VCD::Manager::GetSingleton().CreatePreset(name, editor.current, preset, editor.error)) {
                CloseCreatePresetEditor();
            }
        }

        const auto& error =
            validationError.empty()
            ? editor.error
            : validationError;

        if (!error.empty()) {
            GUI::TextWrapped("%s", error.c_str());
        }

        GUI::End();

        if (wasOpen && !editor.open) {
            CloseCreatePresetEditor();
        }

        GUI::PopStyleColor();
    }

    void __stdcall RenderDynamicsMenu()
    {
        RenderCreateDeleteButtons();

        constexpr auto defaultOpen = GUI::ImGuiTreeNodeFlags_DefaultOpen;

        if (GUI::CollapsingHeader(Trans::Tr("Dynamics.Menu.PlayerDynamicsHeader").c_str(), defaultOpen)) {
            RenderPlayerDynamics();
        }

        GUI::Spacing();

        if (GUI::CollapsingHeader(Trans::Tr("Dynamics.Menu.NPCDynamicsHeader").c_str(), defaultOpen)) {
            RenderNPCDynamics();
        }

        GUI::Separator();

        if (CTAButton(Trans::Tr("Dynamics.Menu.SaveSettingsButton").c_str(), Settings::IsDirty())) {
            Settings::Save();
        }

        WrappedTooltip(
            Trans::Tr("Dynamics.Menu.SaveSettingsTooltip").c_str()
        );

        GUI::SameLine();

        const bool changed = !Settings::IsDynamicsDefault();

        if (CTAButton(Trans::Tr("Dynamics.Menu.ResetDefaultsButton").c_str(), changed)) {

            const auto wasNPCDynamicsEnabled = Settings::GetSettings().enableNPCDynamics;

            Settings::ResetDynamics();

            if (wasNPCDynamicsEnabled) {
                Dynamics::RestoreNPCsToVanilla();
            }

            if (RefreshPresetEditor()) {
                auto& editor = GetPresetEditorState();

                if (editor.npcPreview) {
                    auto actorPtr = editor.previewActor.get();
                    auto* actor = actorPtr.get();

                    if (actor) {
                        VCD::Manager::GetSingleton().SetCollisionData(
                            actor,
                            editor.current,
                            editor.preset,
                            VCD::PresetName(editor.preset),
                            PoseFixes::NPCPose(actor),
                            false
                        );
                    }
                }
                else if (editor.npcGlobal) {
                    if (auto* actor = GetSelectedNPCActorPtr()) {
                        VCD::Manager::GetSingleton().SetCollisionData(
                            actor,
                            editor.current,
                            editor.preset,
                            VCD::PresetName(editor.preset),
                            PoseFixes::NPCPose(actor),
                            false
                        );
                    }
                }
                else {
                    PreviewPreset(editor.preset);
                }
            }
        }

        WrappedTooltip(
            Trans::Tr("Dynamics.Menu.ResetDefaultsTooltip").c_str()
        );

        RenderPresetEditor();
        RenderCreatePresetEditor();
        RenderDeletePresetEditor();
    }

}
