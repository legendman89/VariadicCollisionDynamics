#include "dynamics_panel.hpp"
#include "config.hpp"
#include "tools_panel.hpp"
#include "draw.hpp"
#include "scan.hpp"
#include "translate.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <format>
#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>

namespace UI {

    bool PresetCombo(const char* a_label, VCD::Preset& a_preset)
    {
        const auto& manager = VCD::Manager::GetSingleton();
        const auto& presetConfigs = manager.GetPresetConfigs();
        const auto* current = manager.GetPresetConfig(a_preset);
        bool changed = false;

        SolidBackground(GUI::ImGuiCol_PopupBg);
        GUI::SetNextItemWidth(kFixedComboWidth);
        if (GUI::BeginCombo(a_label, current ? current->name.c_str() : Trans::Tr("Dynamics.Label.Vanilla").c_str())) {
            for (size_t i = 0; i < presetConfigs.size(); ++i) {
                if (i == VCD::kBuiltInPresetCount) {
                    GUI::Separator();
                }
                
                const auto& presetConfig = presetConfigs[i];

                if (manager.IsCameraPreset(presetConfig.preset)) {
                    continue; 
                }

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

    bool CameraPresetCombo(const char* a_label, VCD::Preset& a_preset)
    {
        static constexpr std::array cameraPresets = {
    #define CAMERA_PRESET_COLLECT(S, D) D,
            FOREACH_CAMERA_PRESET_STATE(CAMERA_PRESET_COLLECT)
        };
#undef CAMERA_PRESET_COLLECT
        const auto& manager = VCD::Manager::GetSingleton();
        const auto* current = manager.GetPresetConfig(a_preset);
        bool changed = false;

        SolidBackground(GUI::ImGuiCol_PopupBg);
        GUI::SetNextItemWidth(kFixedComboWidth);
        if (GUI::BeginCombo(a_label, current ? current->name.c_str() : Trans::Tr("Dynamics.Label.Vanilla").c_str())) {
            for (const auto& preset : cameraPresets) {
                const auto* config = manager.GetPresetConfig(preset);
                if (!config) continue;
                const bool selected = preset == a_preset;
                if (GUI::Selectable(config->name.c_str(), selected)) {
                    a_preset = preset;
                    changed = true;
                }
            }
            const auto& presetConfigs = manager.GetPresetConfigs();
            if (presetConfigs.size() > VCD::kBuiltInPresetCount) {
                GUI::Separator();
            }
            for (const auto& config : presetConfigs) {
                if (config.builtIn) {
                    continue;
                }
                const bool selected = config.preset == a_preset;
                if (GUI::Selectable(config.name.c_str(), selected)) {
                    a_preset = config.preset;
                    changed = true;
                }
            }
            GUI::EndCombo();
        }
        GUI::PopStyleColor();
        return changed;
    }

    VCD::Preset GetDefaultNPCActorEditorPreset(const RE::Actor* a_actor)
    {
        if (const auto presetName = VCD::Race::GetSupportedNPCPresetName(a_actor); !presetName.empty()) {
            if (const auto* presetConfig = VCD::Manager::GetSingleton().GetPresetConfigByName(presetName); presetConfig && presetConfig->fileBacked) {
                return presetConfig->preset;
            }
        }

        return VCD::Preset::kVanilla;
    }

    const VCD::CollisionData* GetDefaultNPCActorPresetData(const VCD::Preset& a_preset, const RE::Actor* a_actor)
    {
        if (a_preset == VCD::Preset::kVanilla && a_actor) {
            auto& manager = VCD::Manager::GetSingleton();
            manager.CaptureActorVanillaCollisionData(a_actor);
            if (const auto* actorVanilla = manager.GetActorVanillaCollisionData(a_actor->GetFormID())) {
                return actorVanilla;
            }
        }

        return GetDefaultNPCPresetData(a_preset);
    }

    VCD::Race::CollisionLimitClass GetPresetCollisionLimitClass(const VCD::Preset& a_preset, const RE::Actor* a_actor = nullptr)
    {
        if (a_actor) {
            return VCD::Race::GetCollisionLimitClass(a_actor);
        }

        switch (a_preset) {
        case VCD::Preset::kGiant:
            return VCD::Race::CollisionLimitClass::kGiant;
        case VCD::Preset::kTroll:
        case VCD::Preset::kWerewolf:
        case VCD::Preset::kVampireLord:
            return VCD::Race::CollisionLimitClass::kLargeCreature;
        case VCD::Preset::kVanilla:
        case VCD::Preset::kPersonalSpace:
        case VCD::Preset::kCompact:
        case VCD::Preset::kBulky:
        case VCD::Preset::kSwimming:
        case VCD::Preset::kNPCNeutral:
        case VCD::Preset::kNPCCombat:
        case VCD::Preset::kGuardNeutral:
        case VCD::Preset::kGuardCombat:
        case VCD::Preset::kDraugr:
            return VCD::Race::CollisionLimitClass::kHumanoid;
        case VCD::Preset::kCameraVanilla:
        case VCD::Preset::kCameraDialogue:
        case VCD::Preset::kCameraIndoor:
        case VCD::Preset::kCameraOutdoor:
            return VCD::Race::CollisionLimitClass::kCamera;
        default:
            return VCD::Race::CollisionLimitClass::kDefault;
        }
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
        editor.camera = false;
        editor.autoEnabledDraw = false;
        editor.previousDrawCollision = false;
        editor.previousDrawNearbyActors = false;
        editor.previewActor = {};
        editor.preset = a_preset;
        editor.defaults = *defaultData;
        editor.current = presetConfig->data;
        editor.limits = GetCollisionEditorLimits(VCD::Race::CollisionLimitClass::kPlayer);

        if (editor.preview) {
            if (const auto* player = RE::PlayerCharacter::GetSingleton()) {
                Dynamics::StartPresetPreview(player, a_preset);
            }
        }
    }

    void OpenCameraPresetEditor(const VCD::Preset& a_preset)
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
        editor.preview = true;
        editor.npcPreview = false;
        editor.npcGlobal = false;
        editor.camera = true;
        editor.autoEnabledDraw = false;
        editor.previousDrawCollision = false;
        editor.previousDrawNearbyActors = false;
        editor.previewActor = {};
        editor.preset = a_preset;
        editor.defaults = *defaultData;
        editor.limits = GetCollisionEditorLimits(VCD::Race::CollisionLimitClass::kCamera);
        if (const auto* cameraOverride = Settings::GetCameraPresetOverride(a_preset)) {
            editor.current = *cameraOverride;
        }
        else {
            editor.current = presetConfig->data;
        }

        if (editor.preview) {
            Dynamics::ApplyCameraCollisionRadius(editor.current.capsule.radius);
        }
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

    bool IsEditorCollisionActive()
    {
        auto& editor = GetPresetEditorState();
        if (editor.camera) {
            return false;
        }

        if (editor.npcGlobal || editor.npcPreview) {
            return editor.preview;
        }

        return Dynamics::IsPresetCurrent(editor.preset) || editor.preview;
    }

    bool ApplyPlayerEditorCollision(const bool& a_rebuildConvex)
    {
        auto& editor = GetPresetEditorState();
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player || editor.camera || editor.npcPreview || editor.npcGlobal || !IsEditorCollisionActive()) {
            return false;
        }

        editor.current.RecalculateHeight();
        BeginAutoDraw();
        const auto applied = VCD::Manager::GetSingleton().SetCollisionData(
            player,
            editor.current,
            editor.preset,
            VCD::PresetName(editor.preset),
            PoseFixes::PlayerPose(player),
            a_rebuildConvex,
            a_rebuildConvex
        );

        if (applied && !Dynamics::IsPresetCurrent(editor.preset) && editor.preview) {
            auto& preview = Dynamics::GetPreviewState();
            preview.active = true;
            preview.restorePending = false;
            preview.preset = editor.preset;
        }

        return applied;
    }

    bool ApplyNPCActorEditorCollision(const bool& a_rebuildConvex)
    {
        auto& editor = GetPresetEditorState();
        if (!editor.npcPreview || !editor.preview || !Settings::GetSettings().enableNPCDynamics) {
            return false;
        }

        auto actorPtr = editor.previewActor.get();
        auto* actor = actorPtr.get();
        if (!actor) {
            return false;
        }

        editor.current.RecalculateHeight();
        return VCD::Manager::GetSingleton().SetCollisionData(
            actor,
            editor.current,
            editor.preset,
            VCD::PresetName(editor.preset),
            PoseFixes::NPCPose(actor),
            false,
            a_rebuildConvex
        );
    }

    void ApplyNPCGlobalEditorCollision(const bool& a_rebuildConvex)
    {
        auto& editor = GetPresetEditorState();
        if (!editor.npcGlobal || !editor.preview || !Settings::GetSettings().enableNPCDynamics) {
            return;
        }

        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return;
        }

        auto& npcState = Dynamics::GetNPCDynamicsState();
        auto& scanState = ::Scan::GetNearbyActorScanState();
        const auto radius = Settings::GetSettings().nearbyActorScanRadius;
        const auto radiusSquared = radius * radius;
        std::unordered_map<RE::FormID, bool> seen{};

        if (a_rebuildConvex) {
            npcState.actors.clear();
        }

        auto applyHandle = [&](const RE::ActorHandle& a_handle) {
            auto actorPtr = a_handle.get();
            auto* actor = actorPtr.get();
            if (!actor || seen[actor->GetFormID()]) {
                return;
            }

            seen[actor->GetFormID()] = true;
            if (!Dynamics::CanApplyNPCDynamics(actor, player, radiusSquared)) {
                return;
            }

            const char* stateName = "unknown";
            const auto preset = Dynamics::GetNPCPreset(actor, stateName);
            if (preset != editor.preset) {
                return;
            }

            if (a_rebuildConvex) {
                Dynamics::ApplyNPCPreset(actor, preset, stateName);
                return;
            }

            const auto* actorOverride = Settings::GetNPCActorPresetOverride(actor->GetFormID(), editor.preset);
            const auto& collisionData = actorOverride ? *actorOverride : editor.current;
            VCD::Manager::GetSingleton().SetCollisionData(
                actor,
                collisionData,
                editor.preset,
                VCD::PresetName(editor.preset),
                PoseFixes::NPCPose(actor),
                false,
                false
            );
        };

        editor.current.RecalculateHeight();
        for (auto& handle : npcState.nearbyActors) {
            applyHandle(handle);
        }

        for (auto& handle : scanState.handles) {
            applyHandle(handle);
        }
    }

    bool StartNPCEditorPreview(RE::Actor* a_actor)
    {
        auto& editor = GetPresetEditorState();
        if (!a_actor && !editor.npcGlobal) {
            editor.preview = false;
            editor.previewActor = {};
            return false;
        }

        BeginAutoDraw();
        if (editor.npcGlobal) {
            editor.preview = true;

            auto& preview = Dynamics::GetNPCPreviewState();
            preview.active = true;
            preview.blockActorUpdates = false;
            preview.actor = a_actor ? a_actor->CreateRefHandle() : RE::ActorHandle{};
            preview.preset = editor.preset;
            editor.previewActor = preview.actor;
            ApplyNPCGlobalEditorCollision(false);
            return true;
        }

        editor.preview = true;

        auto& preview = Dynamics::GetNPCPreviewState();
        preview.active = true;
        preview.blockActorUpdates = true;
        preview.actor = a_actor->CreateRefHandle();
        preview.preset = editor.preset;
        editor.previewActor = preview.actor;
        ApplyNPCActorEditorCollision(false);
        return editor.preview;
    }

    void ApplyEditorCollisionPreview()
    {
        auto& editor = GetPresetEditorState();
        if (!IsEditorCollisionActive()) {
            return;
        }

        if (editor.npcPreview || editor.npcGlobal) {
            auto actorPtr = editor.previewActor.get();
            StartNPCEditorPreview(editor.npcPreview ? actorPtr.get() : GetSelectedNPCActorPtr());
        }
        else {
            ApplyPlayerEditorCollision(false);
        }
    }

    void StopNPCEditorPreview()
    {
        auto& editor = GetPresetEditorState();
        Dynamics::StopNPCPresetPreview();
        EndAutoDraw();
        editor.preview = false;
        editor.collisionApplyPending = false;
        editor.collisionApplyAfterRelease = false;
        editor.previewActor = {};
    }

    void ApplyPendingEditorCollision()
    {
        auto& editor = GetPresetEditorState();
        editor.collisionApplyPending = false;
        editor.collisionApplyAfterRelease = false;
        if (editor.npcGlobal) {
            ApplyNPCGlobalEditorCollision(true);
        }
        else if (editor.npcPreview) {
            ApplyNPCActorEditorCollision(true);
        }
        else {
            ApplyPlayerEditorCollision(true);
        }
    }

    void ScheduleEditorCollisionApply(const bool& a_waitForRelease)
    {
        auto& editor = GetPresetEditorState();
        if (!IsEditorCollisionActive()) {
            return;
        }

        editor.collisionApplyPending = true;
        editor.collisionApplyAfterRelease = a_waitForRelease && GUI::IsMouseDown(GUI::ImGuiMouseButton_Left);
        if (!editor.collisionApplyAfterRelease) {
            editor.collisionApplyAt = std::chrono::steady_clock::now() + kEditorCollisionApplyDebounce;
        }
    }

    void TickEditorCollisionApply()
    {
        auto& editor = GetPresetEditorState();
        if (!editor.collisionApplyPending) {
            return;
        }

        if (!editor.open || !IsEditorCollisionActive()) {
            editor.collisionApplyPending = false;
            editor.collisionApplyAfterRelease = false;
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        if (editor.collisionApplyAfterRelease) {
            if (GUI::IsMouseDown(GUI::ImGuiMouseButton_Left)) {
                return;
            }

            editor.collisionApplyAfterRelease = false;
            editor.collisionApplyAt = now + kEditorCollisionApplyDebounce;
            return;
        }

        if (now >= editor.collisionApplyAt) {
            ApplyPendingEditorCollision();
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
        editor.preview = true;
        editor.npcPreview = false;
        editor.npcGlobal = true;
        editor.camera = false;
        editor.autoEnabledDraw = false;
        editor.previousDrawCollision = false;
        editor.previousDrawNearbyActors = false;
        editor.previewActor = {};
        editor.preset = a_preset;
        editor.defaults = *defaultData;
        editor.limits = GetCollisionEditorLimits(GetPresetCollisionLimitClass(a_preset));
        if (const auto* npcOverride = Settings::GetNPCPresetOverride(a_preset)) {
            editor.current = *npcOverride;
        }
        else {
            editor.current = *defaultData;
        }

        StartNPCEditorPreview(GetSelectedNPCActorPtr());
        ScheduleEditorCollisionApply(false);
    }

    void OpenNPCPresetEditor(const VCD::Preset& a_preset, RE::Actor* a_actor)
    {
        if (GetCreatePresetEditorState().open) {
            CloseCreatePresetEditor();
        }

        if (!Settings::GetSettings().enableNPCDynamics) {
            return;
        }

        const auto* defaultData = GetDefaultNPCActorPresetData(a_preset, a_actor);
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
        editor.camera = false;
        editor.autoEnabledDraw = false;
        editor.previousDrawCollision = false;
        editor.previousDrawNearbyActors = false;
        editor.previewActor = a_actor->CreateRefHandle();
        editor.preset = a_preset;
        editor.defaults = *defaultData;
        editor.limits = GetCollisionEditorLimits(GetPresetCollisionLimitClass(a_preset, a_actor));
        if (const auto* actorOverride = Settings::GetNPCActorPresetOverride(a_actor->GetFormID(), a_preset)) {
            editor.current = *actorOverride;
        }
        else {
            editor.current = *defaultData;
        }

        StartNPCEditorPreview(a_actor);
        ScheduleEditorCollisionApply(false);
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
        if (editor.camera) {
            editor.limits = GetCollisionEditorLimits(VCD::Race::CollisionLimitClass::kCamera);
            if (const auto* cameraOverride = Settings::GetCameraPresetOverride(editor.preset)) {
                editor.current = *cameraOverride;
            }
            else {
                editor.current = presetConfig->data;
            }
            return true;
        }

        if (editor.npcGlobal) {
            editor.limits = GetCollisionEditorLimits(GetPresetCollisionLimitClass(editor.preset));
            if (const auto* npcOverride = Settings::GetNPCPresetOverride(editor.preset)) {
                editor.current = *npcOverride;
            }
            else {
                editor.current = *defaultData;
            }
            return true;
        }

        if (editor.npcPreview) {
            auto actorPtr = editor.previewActor.get();
            auto* actor = actorPtr.get();
            if (const auto* defaultNPCData = GetDefaultNPCActorPresetData(editor.preset, actor)) {
                editor.defaults = *defaultNPCData;
            }

            editor.limits = GetCollisionEditorLimits(GetPresetCollisionLimitClass(editor.preset, actor));
            if (actor) {
                if (const auto* actorOverride = Settings::GetNPCActorPresetOverride(actor->GetFormID(), editor.preset)) {
                    editor.current = *actorOverride;
                    return true;
                }
            }
        }

        editor.current = editor.npcPreview ? editor.defaults : presetConfig->data;
        editor.limits = GetCollisionEditorLimits(VCD::Race::CollisionLimitClass::kPlayer);
        return true;
    }

    void RestoreCameraEditorPreview()
    {
        if (!Settings::GetSettings().enableCameraDynamics) {
            Dynamics::RestoreCameraToVanilla();
            return;
        }

        const auto currentCamera = Dynamics::GetPresetState().currentCamera;
        if (currentCamera == VCD::Preset::kCameraDialogue) {
            Dynamics::ApplyCameraPreset(currentCamera);
            return;
        }

        if (const auto* player = RE::PlayerCharacter::GetSingleton()) {
            Dynamics::ApplyCameraPreset(Dynamics::GetCellCameraPreset(player->GetParentCell()));
            return;
        }

        Dynamics::ApplyCameraPreset(Dynamics::GetConfig().cameraNeutral);
    }

    void StopPresetEditorPreview()
    {
        auto& editor = GetPresetEditorState();
        if (editor.collisionApplyPending) {
            ApplyPendingEditorCollision();
        }

        if (editor.camera && editor.preview) {
            RestoreCameraEditorPreview();
        }
        else if ((editor.npcPreview || editor.npcGlobal) && editor.preview) {
            Dynamics::StopNPCPresetPreview();
        }
        else if (editor.preview) {
            if (const auto* player = RE::PlayerCharacter::GetSingleton()) {
                Dynamics::RestorePresetPreview(player);
            }
        }

        EndAutoDraw();
        editor.preview = false;
        editor.collisionApplyPending = false;
        editor.collisionApplyAfterRelease = false;
        editor.npcPreview = false;
        editor.npcGlobal = false;
        editor.camera = false;
    }

    void ClosePresetEditor()
    {
        auto& editor = GetPresetEditorState();
        if (editor.collisionApplyPending) {
            ApplyPendingEditorCollision();
        }

        if (editor.camera && editor.preview) {
            RestoreCameraEditorPreview();
        }
        else if ((editor.npcPreview || editor.npcGlobal) && editor.preview) {
            Dynamics::StopNPCPresetPreview();
        }
        else if (editor.preview) {
            Dynamics::SchedulePreviewRestore(Settings::GetSettings().previewRestoreDelay);
        }

        EndAutoDraw();
        editor.preview = false;
        editor.collisionApplyPending = false;
        editor.collisionApplyAfterRelease = false;
        editor.npcPreview = false;
        editor.npcGlobal = false;
        editor.camera = false;
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
        if (editor.camera) {
            editor.current.RecalculateHeight();
            Settings::MarkCameraPresetEdited(editor.preset, editor.current);
            if (editor.preview) {
                manager.SetCameraCollisionData(editor.current);
            }
        }
        else if (editor.npcPreview) {
            auto actorPtr = editor.previewActor.get();
            auto* actor = actorPtr.get();
            if (actor) {
                editor.current.RecalculateHeight();
                Settings::MarkNPCActorPresetEdited(actor->GetFormID(), VCD::GetActorName(actor), editor.preset, editor.current);
                ApplyEditorCollisionPreview();
            }
        }
        else if (editor.npcGlobal) {
            editor.current.RecalculateHeight();
            Settings::MarkNPCPresetEdited(editor.preset, editor.current);
            if (editor.preview) {
                ApplyEditorCollisionPreview();
            }
        }
        else if (RE::PlayerCharacter::GetSingleton()) {
            presetConfig->data = editor.current;
            presetConfig->data.RecalculateHeight();
            Settings::MarkPresetEdited(editor.preset, presetConfig->data);

            ApplyEditorCollisionPreview();
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
        editor.limits = GetCollisionEditorLimits(VCD::Race::CollisionLimitClass::kPlayer);
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

    CollisionSliderResult RenderCollisionSliders(VCD::CollisionData& a_current, const VCD::CollisionData& a_defaults, const CollisionEditorLimits& a_limits, const bool& isCamera)
    {

        CollisionSliderResult result{};
        GUI::PushStyleColor(GUI::ImGuiCol_FrameBg, Color::kEditFrameBg);
        GUI::PushStyleColor(GUI::ImGuiCol_FrameBgHovered, Color::kEditFrameHover);
        GUI::PushStyleColor(GUI::ImGuiCol_FrameBgActive, Color::kEditFrameActive);
        GUI::PushItemWidth(260.0F);

        const auto kOffsetLimit = a_limits.xyOffset;
        const auto kHeightLimit = a_limits.heightOffset;
        const auto kradiusLimit = a_limits.radius;
        constexpr auto kradiusTolerance = 0.05F;
        constexpr auto kTolerance = 0.1F;
        const auto defaultTop = a_defaults.capsule.point1.z;
        const auto defaultBottom = a_defaults.capsule.point2.z;

        auto radius = a_current.capsule.radius;
        if (GUI::SliderFloat(Trans::Tr("Dynamics.Editor.Radius").c_str(), &radius, kradiusTolerance, kradiusLimit)) {
            a_current.capsule.radius = radius;
            result.changed = true;
        }
        result.active |= GUI::IsItemActive();

        // Grey out top / bottom offset if camera.
        GUI::BeginDisabled(isCamera); 

        auto top = a_current.capsule.point1.z;
        const auto topMin = defaultTop - kHeightLimit;
        const auto topMax = defaultTop + kHeightLimit;
        if (GUI::SliderFloat(Trans::Tr("Dynamics.Editor.TopOffset").c_str(), &top, topMin, topMax)) {
            const auto minTop = std::clamp((a_current.capsule.point2.z + defaultTop) * 0.5F + kTolerance, topMin, topMax);
            a_current.capsule.point1.z = std::clamp(top, minTop, topMax);
            result.changed = true;
        }
        result.active |= GUI::IsItemActive();

        auto bottom = a_current.capsule.point2.z;
        const auto bottomMin = defaultBottom - kHeightLimit;
        const auto bottomMax = defaultBottom + kHeightLimit;
        if (GUI::SliderFloat(Trans::Tr("Dynamics.Editor.BottomOffset").c_str(), &bottom, bottomMin, bottomMax)) {
            const auto maxBottom = std::clamp((a_current.capsule.point1.z + defaultBottom) * 0.5F - kTolerance, bottomMin,  bottomMax);
            a_current.capsule.point2.z = std::clamp(bottom, bottomMin, maxBottom);
            result.changed = true;
        }
        result.active |= GUI::IsItemActive();

        GUI::EndDisabled(); 

        auto forward = a_current.bump.translation.y;
        if (GUI::SliderFloat(Trans::Tr("Dynamics.Editor.ForwardOffset").c_str(), &forward, -kOffsetLimit, kOffsetLimit)) {
            a_current.bump.translation.y = forward;
            result.changed = true;
        }
        result.active |= GUI::IsItemActive();

        auto side = a_current.bump.translation.x;
        if (GUI::SliderFloat(Trans::Tr("Dynamics.Editor.SideOffset").c_str(), &side, -kOffsetLimit, kOffsetLimit)) {
            a_current.bump.translation.x = side;
            result.changed = true;
        }
        result.active |= GUI::IsItemActive();

        GUI::PopItemWidth();
        GUI::PopStyleColor(3);
        return result;
    }

    void RenderStateRow(const char* a_label, VCD::Preset& a_preset, const bool& a_previewPlayer = true, const bool& a_showEdit = true, const bool& a_npcGlobal = false)
    {
        GUI::TableNextRow();
        GUI::TableNextColumn();
        GUI::Text("%s", a_label);
        GUI::TableNextColumn();
        GUI::SetNextItemWidth(kFixedComboWidth);
        if (PresetCombo((std::string("##") + a_label).c_str(), a_preset) && a_previewPlayer) {
            PreviewPreset(a_preset);
        }
        if (a_showEdit) {
            GUI::SameLine(0, 12.0F);
            if (EditButton(a_label)) {
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
        }
    }

    void RenderCameraStateRow(const char* a_label, VCD::Preset& a_preset)
    {
        GUI::TableNextRow();
        GUI::TableNextColumn();
        GUI::Text("%s", a_label);
        GUI::TableNextColumn();
        GUI::SetNextItemWidth(kFixedComboWidth);
        if (CameraPresetCombo((std::string("##") + a_label).c_str(), a_preset) && Settings::GetSettings().enableCameraDynamics) {
            Dynamics::ApplyCameraPreset(a_preset);
        }
        GUI::SameLine();
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonHovered, Color::kEditHover);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonActive, Color::kEditActive);
         if (EditButton(a_label)) {
            OpenCameraPresetEditor(a_preset);
        }
        GUI::PopStyleColor(2);
    }

    void RenderNPCActorSelector(VCD::Preset& a_selectedPreset)
    {
        auto options = Scan::GetNearbyNPCActorOptions();
        auto* selectedActor = GetSelectedNPCActorPtr();
        std::string preview = Trans::Tr("Dynamics.NPC.SelectActorPreview");

        if (!selectedActor && !options.empty()) {
            GetSelectedNPCActor() = options.front().handle;
            selectedActor = GetSelectedNPCActorPtr();
            a_selectedPreset = GetDefaultNPCActorEditorPreset(selectedActor);
        }

        if (selectedActor) {
            preview = VCD::GetActorName(selectedActor);
            for (auto& option : options) {
                auto actorPtr = option.handle.get();
                if (actorPtr.get() == selectedActor) {
                    preview = option.label;
                    break;
                }
            }
        }

        
        SolidBackground(GUI::ImGuiCol_PopupBg);
        GUI::SetNextItemWidth(kFixedActorComboWidth);
        if (GUI::BeginCombo(Trans::Tr("Dynamics.NPC.ActorComboLabel").c_str(), preview.c_str())) {
            for (auto& option : options) {
                auto actorPtr = option.handle.get();
                const auto selected = selectedActor && actorPtr.get() == selectedActor;

                if (GUI::Selectable(option.label.c_str(), selected)) {
                    GetSelectedNPCActor() = option.handle;
                    a_selectedPreset = GetDefaultNPCActorEditorPreset(actorPtr.get());
                }
            }
            GUI::EndCombo();
        }

        GUI::PopStyleColor(); // Pop Bg color

        Tooltip(Trans::Tr("Dynamics.NPC.ActorComboTooltip").c_str());

        GUI::SameLine(0.0F, 12.0F);

        if (EditTextButton(Trans::Tr("Dynamics.NPC.EditSelectedActorButton").c_str())) {
            OpenNPCPresetEditor(a_selectedPreset, GetSelectedNPCActorPtr());
        }

        Tooltip(Trans::Tr("Dynamics.NPC.EditSelectedActorTooltip").c_str());
    }

    void RenderCreateDeleteButtons()
    {
        static const auto circlePlusText = FontAwesome::UnicodeToUtf8(Icons::kCirclePlus);
        const auto createPresetWidth = PresetActionButtonWidth(Trans::Tr("Dynamics.CreatePreset.Button").c_str());
        const auto deletePresetWidth = PresetActionButtonWidth(Trans::Tr("Dynamics.DeletePreset.Button").c_str());

        GUI::PushStyleVar(GUI::ImGuiStyleVar_FrameRounding, 5.0F);
        GUI::PushStyleVar(GUI::ImGuiStyleVar_FrameBorderSize, 1.0F);
        GUI::PushStyleColor(GUI::ImGuiCol_Button, UI::Color::kCreatePresetBG);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonHovered, UI::Color::kCreatePresetHover);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonActive, UI::Color::kCreatePresetActive);
        GUI::PushStyleColor(GUI::ImGuiCol_Text, UI::Color::kCreatePresetText);
        GUI::PushStyleColor(GUI::ImGuiCol_Border, UI::Color::kCreatePresetBorder);

        const bool createPreset =
            GUI::Button(
                ("        " + Trans::Tr("Dynamics.CreatePreset.Button") + "##CreateNewPreset").c_str(),
                GUI::ImVec2{ createPresetWidth, kPresetActionButtonHeight }
            );

        WrappedTooltip(
            Trans::Tr("Dynamics.CreatePreset.Tooltip").c_str()
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

        const auto* style = GUI::GetStyle();
        GUI::SetCursorScreenPos(GUI::ImVec2{ buttonMax.x + (style ? style->ItemSpacing.x : 8.0F), buttonMin.y });

        static const auto circleMinusText = FontAwesome::UnicodeToUtf8(Icons::kCircleMinus);
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

        const bool deletePreset =
            GUI::Button(
                ("        " + Trans::Tr("Dynamics.DeletePreset.Button") + "##DeletePreset").c_str(),
                GUI::ImVec2{ deletePresetWidth, kPresetActionButtonHeight }
            );
        WrappedTooltip(hasCustomPresets ? Trans::Tr("Dynamics.DeletePreset.Tooltip.Custom").c_str() :
                                          Trans::Tr("Dynamics.DeletePreset.Tooltip.Default").c_str());

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
    }

    void ResetDynamicsToDefaults()
    {
        const auto wasNPCDynamicsEnabled = Settings::GetSettings().enableNPCDynamics;
        const auto wasCameraDynamicsEnabled = Settings::GetSettings().enableCameraDynamics;

        Settings::ResetDynamics();

        if (wasNPCDynamicsEnabled) {
            Dynamics::RestoreNPCsToVanilla();
        }
        if (wasCameraDynamicsEnabled) {
            Dynamics::RestoreCameraToVanilla();
        }

        if (RefreshPresetEditor()) {
            auto& editor = GetPresetEditorState();

            if (editor.camera && editor.preview) {
                Dynamics::ApplyCameraCollisionRadius(editor.current.capsule.radius);
            }
            else if (editor.npcPreview) {
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

    void RenderDynamicsSaveResetButtons()
    {
        if (IconCTAButton(Trans::Tr("Dynamics.Menu.SaveSettingsButton").c_str(), Settings::IsDirty(), Icons::kSave)) {
            Settings::Save();
        }

        WrappedTooltip(
            Trans::Tr("Dynamics.Menu.SaveSettingsTooltip").c_str()
        );

        GUI::SameLine(0.0F, 6.0F);

        const bool changed = !Settings::IsDynamicsDefault();

        if (IconCTAButton(Trans::Tr("Dynamics.Menu.ResetDefaultsButton").c_str(), changed, Icons::kReset)) {
            ResetDynamicsToDefaults();
        }

        WrappedTooltip(
            Trans::Tr("Dynamics.Menu.ResetDefaultsTooltip").c_str()
        );
    }

    void RenderDynamicsActionBar()
    {
        const auto* style = GUI::GetStyle();
        const auto spacing = style ? style->ItemSpacing.x : 8.0F;
        const auto saveWidth = IconCTAButtonWidth(Trans::Tr("Dynamics.Menu.SaveSettingsButton").c_str());
        const auto resetWidth = IconCTAButtonWidth(Trans::Tr("Dynamics.Menu.ResetDefaultsButton").c_str());
        const auto rightGroupWidth = saveWidth + 6.0F + resetWidth;
        const auto createPresetWidth = PresetActionButtonWidth(Trans::Tr("Dynamics.CreatePreset.Button").c_str());
        const auto deletePresetWidth = PresetActionButtonWidth(Trans::Tr("Dynamics.DeletePreset.Button").c_str());
        const auto presetGroupWidth = createPresetWidth + spacing + deletePresetWidth;

        RenderActionBar(presetGroupWidth, kPresetActionButtonHeight, rightGroupWidth, IconCTAButtonHeight(), RenderCreateDeleteButtons, RenderDynamicsSaveResetButtons);
    }

    void RenderDynamicsSections()
    {
        constexpr auto defaultOpen = GUI::ImGuiTreeNodeFlags_DefaultOpen;

        if (GUI::CollapsingHeader(Trans::Tr("Dynamics.Menu.PlayerDynamicsHeader").c_str(), defaultOpen)) {
            RenderPlayerDynamics();
        }

        GUI::Spacing();

        if (GUI::CollapsingHeader(Trans::Tr("Dynamics.Menu.NPCDynamicsHeader").c_str(), defaultOpen)) {
            RenderNPCDynamics();
        }

        GUI::Separator();

        if (GUI::CollapsingHeader(Trans::Tr("Dynamics.Menu.CameraDynamicsHeader").c_str(), defaultOpen)) {
            RenderCameraDynamics();
        }
    }

    void RenderPlayerDynamics()
    {
        auto& config = Dynamics::GetConfig();

        if (GUI::BeginTable("PlayerDynamicsPresetTable", 2)) {

            GUI::TableSetupColumn(Trans::Tr("Dynamics.PlayerDynamics.Column.State").c_str(), GUI::ImGuiTableColumnFlags_WidthFixed, kFixedDynamicsStateColumnWidth);
            GUI::TableSetupColumn(Trans::Tr("Dynamics.PlayerDynamics.Column.Preset").c_str(), GUI::ImGuiTableColumnFlags_WidthStretch);

            RenderStateRow(Trans::Tr("Dynamics.PlayerDynamics.State.Werewolf").c_str(), config.werewolf);
            RenderStateRow(Trans::Tr("Dynamics.PlayerDynamics.State.VampireLord").c_str(), config.vampireLord);
            RenderStateRow(Trans::Tr("Dynamics.PlayerDynamics.State.Swimming").c_str(), config.swimming);
            RenderStateRow(Trans::Tr("Dynamics.PlayerDynamics.State.Combat").c_str(), config.combat);
            RenderStateRow(Trans::Tr("Dynamics.PlayerDynamics.State.Outdoor").c_str(), config.outdoor);
            RenderStateRow(Trans::Tr("Dynamics.PlayerDynamics.State.Indoor").c_str(), config.indoor);
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

        GUI::SetNextItemWidth(kFixedSliderWidth);
        GUI::SliderFloat(Trans::Tr("Dynamics.NPC.NearbyActorRadius").c_str(), &settings.nearbyActorScanRadius, 256.0F, 8192.0F);

        GUI::SetNextItemWidth(kFixedSliderWidth);
        GUI::SliderFloat(Trans::Tr("Dynamics.NPC.NearbyActorScanInterval").c_str(), &settings.nearbyActorScanInterval, 0.1F, 2.0F);

        GUI::SetNextItemWidth(kFixedSliderWidth);
        GUI::SliderInt(Trans::Tr("Dynamics.NPC.NearbyActorLimit").c_str(), &settings.nearbyActorScanLimit, 1, 64);

        GUI::BeginDisabled(!settings.enableNPCDynamics);

        if (GUI::BeginTable("NPCDynamicsPresetTable", 2)) {

            GUI::TableSetupColumn(Trans::Tr("Dynamics.NPC.Column.State").c_str(), GUI::ImGuiTableColumnFlags_WidthFixed, kFixedDynamicsStateColumnWidth);
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

    void RenderCameraDynamics()
    {
        auto& config = Dynamics::GetConfig();
        auto& settings = Settings::GetSettings();

        if (GUI::Checkbox(Trans::Tr("Dynamics.Camera.EnableCheckbox").c_str(), &settings.enableCameraDynamics)
            && !settings.enableCameraDynamics)
        {
            Dynamics::RestoreCameraToVanilla();
            auto& editor = GetPresetEditorState();
            if (editor.camera) {
                ClosePresetEditor();
                editor.open = false;
            }
        }

        GUI::BeginDisabled(!settings.enableCameraDynamics);

        if (GUI::BeginTable("CameraDynamicsTable", 2)) {
            GUI::TableSetupColumn(Trans::Tr("Dynamics.Camera.Column.State").c_str(), GUI::ImGuiTableColumnFlags_WidthFixed, kFixedDynamicsStateColumnWidth);
            GUI::TableSetupColumn(Trans::Tr("Dynamics.Camera.Column.Preset").c_str(), GUI::ImGuiTableColumnFlags_WidthStretch);
            RenderCameraStateRow(Trans::Tr("Dynamics.Camera.State.Indoor").c_str(), config.cameraIndoor);
            RenderCameraStateRow(Trans::Tr("Dynamics.Camera.State.Outdoor").c_str(), config.cameraOutdoor);
            RenderCameraStateRow(Trans::Tr("Dynamics.Camera.State.Dialogue").c_str(), config.cameraDialogue);

            GUI::EndTable();
        }

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
            editor.camera ? std::string("Camera")
            : (editor.npcPreview ? VCD::GetActorName(GetSelectedNPCActorPtr())
            : (editor.npcGlobal ? std::string("NPC") : std::string("Player")));

        const auto title = Trans::Tr("Dynamics.Editor.WindowTitlePrefix") + " " +
            actorName + " " + 
            Trans::Tr("Dynamics.Editor.WindowTitlePreset") + " " + 
            VCD::PresetName(editor.preset) +
            "###VCDPresetEditor";


        GUI::SetNextWindowSize(kPresetEditorWindowSize, GUI::ImGuiCond_Appearing);
        CenterNextWindow();

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

        if (editor.camera) {
            if (!editor.preview) {
                editor.preview = true;
                Dynamics::ApplyCameraCollisionRadius(editor.current.capsule.radius);
            }
        }
        else if (editor.npcGlobal) {
            const std::string previewLabel =
                Trans::Tr("Dynamics.Editor.PreviewPrefix") + " " +
                actorName + " " +
                Trans::Tr("Dynamics.Editor.PresetLabel");

            if (GUI::Checkbox(previewLabel.c_str(), &editor.preview)) {
                if (editor.preview) {
                    StartNPCEditorPreview(GetSelectedNPCActorPtr());
                    ScheduleEditorCollisionApply(false);
                }
                else {
                    StopNPCEditorPreview();
                }
            }

            WrappedTooltip(Trans::Tr("Dynamics.Editor.PreviewTooltip").c_str());
        }
        else if (!isCurrentPreset) {
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
                        ApplyEditorCollisionPreview();
                        ScheduleEditorCollisionApply(false);
                    }
                    else {
                        editor.collisionApplyPending = false;
                        editor.collisionApplyAfterRelease = false;
                        Dynamics::RestorePresetPreview(player);
                        EndAutoDraw();
                    }
                }
            }

            WrappedTooltip(
                Trans::Tr("Dynamics.Editor.PreviewTooltip").c_str()
            );
        }

        if (editor.npcPreview) {
            auto actorPtr = editor.previewActor.get();
            auto* actor = actorPtr.get();
            auto preset = editor.preset;

            GUI::SetNextItemWidth(kFixedPreviewComboWidth);
            if (PresetCombo(Trans::Tr("Dynamics.NPC.PreviewPresetLabel").c_str(), preset) && actor) {
                if (const auto* defaultData = GetDefaultNPCActorPresetData(preset, actor)) {
                    editor.preset = preset;
                    editor.defaults = *defaultData;
                    editor.limits = GetCollisionEditorLimits(GetPresetCollisionLimitClass(preset, actor));
                    if (const auto* actorOverride = Settings::GetNPCActorPresetOverride(actor->GetFormID(), preset)) {
                        editor.current = *actorOverride;
                    }
                    else {
                        editor.current = *defaultData;
                    }
                    StartNPCEditorPreview(actor);
                    ScheduleEditorCollisionApply(false);
                }
            }
        }

        /*if (editor.camera) {
            GUI::PushStyleColor(GUI::ImGuiCol_FrameBg, Color::kEditFrameBg);
            GUI::PushStyleColor(GUI::ImGuiCol_FrameBgHovered, Color::kEditFrameHover);
            GUI::PushStyleColor(GUI::ImGuiCol_FrameBgActive, Color::kEditFrameActive);
            GUI::PushItemWidth(260.0F);
            auto radius = editor.current.capsule.radius;
            if (GUI::SliderFloat(Trans::Tr("Dynamics.Editor.Radius").c_str(), &radius, 0.05F, 30.0F)) {
                editor.current.capsule.radius = radius;
                UpdateEditedPreset();
            }


            GUI::PopItemWidth();
            GUI::PopStyleColor(3);

        }
        else {*/
            const auto sliderResult = RenderCollisionSliders(editor.current, editor.defaults, editor.limits, editor.camera);
            if (sliderResult.changed) {
                UpdateEditedPreset();
                ScheduleEditorCollisionApply(sliderResult.active);
            }
      //  }

        GUI::Spacing();

        const bool differsFromDefault = !editor.current.IsSame(editor.defaults);

        if (IconCTAButton(Trans::Tr("Dynamics.Editor.ResetButton").c_str(), differsFromDefault, Icons::kReset)) {
            if (editor.camera) {
                Settings::ClearCameraPresetEdited(editor.preset);
                editor.current = editor.defaults;
                if (editor.preview) {
                    Dynamics::ApplyCameraCollisionRadius(editor.current.capsule.radius);
                }
            }
            else if (editor.npcPreview) {
                auto actorPtr = editor.previewActor.get();
                auto* actor = actorPtr.get();

                if (actor) {
                    Settings::ClearNPCActorPresetEdited(actor->GetFormID(), editor.preset);
                    editor.current = editor.defaults;
                    StartNPCEditorPreview(actor);
                    ScheduleEditorCollisionApply(false);
                }
            }
            else if (editor.npcGlobal) {
                Settings::ClearNPCPresetEdited(editor.preset);

                editor.current = editor.defaults;

                if (editor.preview) {
                    StartNPCEditorPreview(GetSelectedNPCActorPtr());
                    ScheduleEditorCollisionApply(false);
                }
            }
            else if (RE::PlayerCharacter::GetSingleton()) {
                presetConfig->data = editor.defaults;

                Settings::ClearPresetEdited(editor.preset);
                editor.current = editor.defaults;

                if (Dynamics::IsPresetCurrent(editor.preset)) {
                    ApplyEditorCollisionPreview();
                    ScheduleEditorCollisionApply(false);
                }
                else if (editor.preview) {
                    ApplyEditorCollisionPreview();
                    ScheduleEditorCollisionApply(false);
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

        GUI::SetNextWindowSize(kDeletePresetEditorWindowSize, GUI::ImGuiCond_Appearing);
        CenterNextWindow();

        const auto wasOpen = editor.open;
        GUI::PushStyleColor(GUI::ImGuiCol_WindowBg, Color::kEditWindowBg);
        if (!GUI::Begin(Trans::Tr("Dynamics.DeletePreset.WindowTitle").c_str(), &editor.open, 0)) {
            GUI::End();
            if (wasOpen && !editor.open) {
                CloseDeletePresetEditor();
            }
            GUI::PopStyleColor();
            return;
        }

        SolidBackground(GUI::ImGuiCol_PopupBg);
        GUI::SetNextItemWidth(kFixedDeletePresetComboWidth);
        if (GUI::BeginCombo(Trans::Tr("Dynamics.DeletePreset.ComboName").c_str(), selected->name.c_str())) {
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

        GUI::Spacing();
        GUI::TextWrapped(Trans::Tr("Dynamics.DeletePreset.TextTip").c_str());
        GUI::Spacing();
        GUI::Spacing();

        if (CTAButton(Trans::Tr("Dynamics.DeletePreset.DeleteButton").c_str(), true)) {
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
        GUI::SetCursorPosX(GUI::GetCursorPosX() + 14.0F);
        if (GUI::Button(Trans::Tr("Dynamics.DeletePreset.CancelButton").c_str())) {
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

        GUI::Spacing();
    }

    void RenderCreatePresetEditor()
    {
        auto& editor = GetCreatePresetEditorState();
        if (!editor.open) {
            return;
        }

        GUI::SetNextWindowSize(kCreatePresetEditorWindowSize, GUI::ImGuiCond_Appearing);
        CenterNextWindow();

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

        GUI::SetNextItemWidth(kFixedCreatePresetInputWidth);
        if (GUI::InputText(Trans::Tr("Dynamics.CreatePreset.NameLabel").c_str(), editor.name.data(), editor.name.size())) {
            editor.error.clear();
        }

        if (GUI::Checkbox(Trans::Tr("Dynamics.CreatePreset.PreviewCheckbox").c_str(), &editor.preview)) {
            if (editor.preview)
                ApplyCreatePresetPreview();
            else
                RestoreCreatePresetPreview();
        }

        //LEGENDMAN (safe to pass false for is camera here ?)
        const auto sliderResult = RenderCollisionSliders(editor.current, editor.defaults, editor.limits, false);
        if (sliderResult.changed) {
            ApplyCreatePresetPreview();
        }

        GUI::Spacing();

        const bool differsFromDefault = !editor.current.IsSame(editor.defaults);

        if (IconCTAButton(Trans::Tr("Dynamics.CreatePreset.DefaultButton").c_str(), differsFromDefault, Icons::kReset)) {
            editor.current = editor.defaults;
            ApplyCreatePresetPreview();
        }

        GUI::SameLine(0.0F, 6.0F);

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

        if (IconCTAButton(Trans::Tr("Dynamics.CreatePreset.SaveButton").c_str(), canSave, Icons::kSave)) {
            VCD::Preset preset{};

            if (VCD::Manager::GetSingleton().CreatePreset(name, editor.current, preset, editor.error)) {
                CloseCreatePresetEditor();
            }
        }

        const auto& error = validationError.empty() ? editor.error : validationError;

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
        TickEditorCollisionApply();

        RenderDynamicsActionBar();

        GUI::ImVec2 scrollSize{};
        GUI::GetContentRegionAvail(&scrollSize);
        if (GUI::BeginChild("DynamicsScrollRegion", scrollSize, GUI::ImGuiChildFlags_None, 0)) {
            RenderDynamicsSections();
        }
        GUI::EndChild();

        RenderPresetEditor();
        RenderCreatePresetEditor();
        RenderDeletePresetEditor();
    }

}
