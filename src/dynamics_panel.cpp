#include "dynamics_panel.hpp"
#include "tools_panel.hpp"
#include "draw.hpp"

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
        auto index = static_cast<int>(VCD::ToUnderlying(a_preset));

        SolidBackground(GUI::ImGuiCol_PopupBg);
        const bool changed = GUI::Combo(a_label, &index, VCD::kPresetNames, static_cast<int>(std::size(VCD::kPresetNames)));
        GUI::PopStyleColor();

        if (!changed) {
            return false;
        }

        a_preset = static_cast<VCD::Preset>(index);
        return true;
    }

    void OpenPresetEditor(const VCD::Preset& a_preset)
    {
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
                manager.SetCollisionData(actor, editor.current, editor.preset, VCD::PresetName(editor.preset), false);
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
                manager.SetPreset(player, editor.preset);
            }
            else if (editor.preview) {
                BeginAutoDraw();
                Dynamics::StartPresetPreview(player, editor.preset);
            }
        }
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
            if (GUI::Button((std::string("Edit##") + a_label).c_str())) {
                if (a_npcGlobal) {
                    OpenNPCGlobalPresetEditor(a_preset);
                }
                else {
                    OpenPresetEditor(a_preset);
                }
            }
            Tooltip(a_npcGlobal ? "Edit this preset globally for NPCs. To fine tune one actor, use Edit Selected Actor below." : "Open sliders for the preset assigned to this state.");
            GUI::PopStyleColor(2);
        }
    }

    void RenderNPCActorSelector(VCD::Preset& a_selectedPreset)
    {
        auto options = GetNearbyNPCActorOptions();
        auto* selectedActor = GetSelectedNPCActorPtr();
        std::string preview = "Select nearby actor";

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
        if (GUI::BeginCombo("Actor", preview.c_str())) {
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
        Tooltip("Select one nearby actor for preset preview and editing.");

        GUI::SameLine();
        GUI::SetNextItemWidth(180.0F);
        PresetCombo("Preview Preset", a_selectedPreset);

        GUI::SameLine();
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonHovered, Color::kEditHover);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonActive, Color::kEditActive);
        if (GUI::Button("Edit Selected Actor")) {
            OpenNPCPresetEditor(a_selectedPreset, GetSelectedNPCActorPtr());
        }
        Tooltip("Open sliders for this preset and preview them on the selected actor.");
        GUI::PopStyleColor(2);
    }

    void RenderPlayerDynamics()
    {
        auto& config = Dynamics::GetConfig();

        if (GUI::BeginTable("PlayerDynamicsPresetTable", 2)) {
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
    }

    void RenderNPCDynamics()
    {
        auto& config = Dynamics::GetConfig();
        auto& settings = Settings::GetSettings();

        if (GUI::Checkbox("Enable NPC Dynamics", &settings.enableNPCDynamics) && !settings.enableNPCDynamics) {
            Dynamics::RestoreNPCsToVanillaLike();
        }

        if (GUI::BeginTable("NPCDynamicsPresetTable", 2)) {
            GUI::TableSetupColumn("State", GUI::ImGuiTableColumnFlags_WidthFixed, 120.0F);
            GUI::TableSetupColumn("Preset", GUI::ImGuiTableColumnFlags_WidthStretch);

            RenderStateRow("NPC Neutral", config.npcNeutral, false, true, true);
            RenderStateRow("NPC Combat", config.npcCombat, false, true, true);
            RenderStateRow("Guard Neutral", config.guardNeutral, false, true, true);
            RenderStateRow("Guard Combat", config.guardCombat, false, true, true);

            GUI::EndTable();
        }

        GUI::Spacing();
        static auto selectedPreset = VCD::Preset::kVanillaLike;
        RenderNPCActorSelector(selectedPreset);
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

        const auto actorName = editor.npcPreview ? GetActorName(GetSelectedNPCActorPtr()) : (editor.npcGlobal ? std::string("NPC") : std::string("Player"));
        const auto title = std::string("Edit " + actorName + " Preset: ") + VCD::PresetName(editor.preset);
        constexpr auto windowSize = GUI::ImVec2{ 540.0F, 320.0F };
        GUI::SetNextWindowSize(windowSize, GUI::ImGuiCond_Appearing);
        if (const auto* io = GUI::GetIO()) {
            GUI::SetNextWindowPos(GUI::ImVec2{ io->DisplaySize.x * 0.5F, io->DisplaySize.y * 0.5F }, GUI::ImGuiCond_Appearing, GUI::ImVec2{ 0.5F, 0.5F });
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
            const std::string previewLabel = "Preview " + actorName + " Preset";
            if (!editor.npcPreview && !editor.npcGlobal && GUI::Checkbox(previewLabel.c_str(), &editor.preview)) {
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
            WrappedTooltip("Temporarily applies this preset for tuning. Closing the editor restores the current dynamics state after a short delay.");
        }

        GUI::PushStyleColor(GUI::ImGuiCol_FrameBg, Color::kEditFrameBg);
        GUI::PushStyleColor(GUI::ImGuiCol_FrameBgHovered, Color::kEditFrameHover);
        GUI::PushStyleColor(GUI::ImGuiCol_FrameBgActive, Color::kEditFrameActive);
        GUI::PushItemWidth(260.0F);

        constexpr auto kOffsetLimit = 30.0F;
        constexpr auto kHeightLimit = 3.0F;
        const auto defaultTop = editor.defaults.capsule.point1.z;
        const auto defaultBottom = editor.defaults.capsule.point2.z;

        auto radius = editor.current.capsule.radius;
        if (GUI::SliderFloat("Radius", &radius, 0.05F, 1.0F)) {
            editor.current.capsule.radius = radius;
            UpdateEditedPreset();
        }

        auto topOffset = editor.current.capsule.point1.z - defaultTop;
        topOffset = std::clamp(topOffset, 0.0F, kHeightLimit);
        if (GUI::SliderFloat("Top Offset", &topOffset, 0.0F, kHeightLimit)) {
            editor.current.capsule.point1.z = defaultTop + topOffset;
            UpdateEditedPreset();
        }

        auto bottomOffset = defaultBottom - editor.current.capsule.point2.z;
        bottomOffset = std::clamp(bottomOffset, 0.0F, kHeightLimit);
        if (GUI::SliderFloat("Bottom Offset", &bottomOffset, 0.0F, kHeightLimit)) {
            editor.current.capsule.point2.z = defaultBottom - bottomOffset;
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
        GUI::PopStyleColor(3);

        const bool differsFromDefault = !editor.current.IsSame(editor.defaults);
        if (CTAButton("Default", differsFromDefault)) {
            if (editor.npcPreview) {
                auto actorPtr = editor.previewActor.get();
                auto* actor = actorPtr.get();
                if (actor) {
                    BeginAutoDraw();
                    Settings::ClearNPCActorPresetEdited(actor->GetFormID(), editor.preset);
                    editor.current = editor.defaults;
                    manager.SetCollisionData(actor, editor.current, editor.preset, VCD::PresetName(editor.preset), false);
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
                    manager.SetPreset(player, editor.preset);
                }
                else if (editor.preview) {
                    BeginAutoDraw();
                    Dynamics::StartPresetPreview(player, editor.preset);
                }
            }
        }
        Tooltip("Restore this editor to its base preset values without saving it to disk.");

        GUI::End();
        if (wasOpen && !editor.open) {
            ClosePresetEditor();
        }
        GUI::PopStyleColor();
    }

    void __stdcall RenderDynamicsMenu()
    {
        constexpr auto defaultOpen = GUI::ImGuiTreeNodeFlags_DefaultOpen;
        if (GUI::CollapsingHeader("Player Dynamics", defaultOpen)) {
            RenderPlayerDynamics();
        }

        GUI::Spacing();

        if (GUI::CollapsingHeader("NPC Dynamics", defaultOpen)) {
            RenderNPCDynamics();
        }

        GUI::Separator();

        if (CTAButton("Save Settings", Settings::IsDirty())) {
            Settings::Save();
        }
        Tooltip("Save state mappings and edited presets to disk.");

        GUI::SameLine();

        const bool changed = !Settings::IsDynamicsDefault();

        if (CTAButton("Default", changed)) {

            const auto wasNPCDynamicsEnabled = Settings::GetSettings().enableNPCDynamics;
            Settings::ResetDynamics();
            if (wasNPCDynamicsEnabled) {
                Dynamics::RestoreNPCsToVanillaLike();
            }

            if (RefreshPresetEditor()) {
                auto& editor = GetPresetEditorState();
                if (editor.npcPreview) {
                    auto actorPtr = editor.previewActor.get();
                    auto* actor = actorPtr.get();
                    if (actor) {
                        VCD::Manager::GetSingleton().SetCollisionData(actor, editor.current, editor.preset, VCD::PresetName(editor.preset), false);
                    }
                }
                else if (editor.npcGlobal) {
                    if (auto* actor = GetSelectedNPCActorPtr()) {
                        VCD::Manager::GetSingleton().SetCollisionData(actor, editor.current, editor.preset, VCD::PresetName(editor.preset), false);
                    }
                }
                else {
                    PreviewPreset(editor.preset);
                }
            }
        }
        Tooltip("Reset dynamics state mappings and preset overrides to defaults without saving.");

        RenderPresetEditor();
    }

}
