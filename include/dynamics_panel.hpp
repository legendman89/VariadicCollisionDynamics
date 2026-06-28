#pragma once

#include "menu.hpp"
#include "color.hpp"
#include "logger.hpp"
#include "helper.hpp"
#include "manager.hpp"
#include "dynamics.hpp"
#include "settings.hpp"
#include "posefixes.hpp"

#include <array>

namespace UI {

    struct PresetEditorState
    {
        bool open{ false };
        bool preview{ false };
        bool npcPreview{ false };
        bool npcGlobal{ false };
        bool autoEnabledDraw{ false };
        bool previousDrawCollision{ false };
        bool previousDrawNearbyActors{ false };

        RE::ActorHandle previewActor{};
        VCD::Preset preset{ VCD::Preset::kVanilla };
        VCD::CollisionData defaults{};
        VCD::CollisionData current{};
    };

    struct CreatePresetEditorState
    {
        bool open{ false };
        bool preview{ false };
        std::array<char, 128> name{};
        std::string error{};
        VCD::CollisionData defaults{};
        VCD::CollisionData current{};
    };

    struct DeletePresetEditorState
    {
        bool open{ false };
        VCD::Preset preset{ VCD::Preset::kVanilla };
        std::string error{};
    };

    struct NPCActorOption
    {
        RE::ActorHandle handle{};
        std::string name{};
        std::string label{};
        RE::FormID formID{ 0 };
    };

    inline PresetEditorState& GetPresetEditorState()
    {
        static PresetEditorState state{};
        return state;
    }

    inline CreatePresetEditorState& GetCreatePresetEditorState()
    {

        static CreatePresetEditorState state{};
        return state;
    }

    inline DeletePresetEditorState& GetDeletePresetEditorState()
    {
        static DeletePresetEditorState state{};
        return state;
    }

    inline VCD::Preset& GetSelectedNPCPreset()
    {
        static VCD::Preset preset{ VCD::Preset::kVanilla };
        return preset;
    }

    inline void PreviewPreset(const VCD::Preset& a_preset)
    {
        if (const auto* player = RE::PlayerCharacter::GetSingleton()) {
            logger::info("Preset preview result: {}", VCD::Manager::GetSingleton().SetPreset(player, a_preset, PoseFixes::PlayerPose(player), true));
        }
    }

    inline RE::ActorHandle& GetSelectedNPCActor()
    {
        static RE::ActorHandle actor{};
        return actor;
    }

    inline RE::Actor* GetSelectedNPCActorPtr()
    {
        auto& selected = GetSelectedNPCActor();
        auto actorPtr = selected.get();
        return actorPtr.get();
    }

    inline std::string GetActorName(RE::Actor* a_actor)
    {
        if (!a_actor) {
            return "Actor";
        }

        const auto* name = a_actor->GetDisplayFullName();
        if (!name || std::strlen(name) == 0) {
            return "Actor";
        }

        return name;
    }

    inline const VCD::CollisionData* GetDefaultPresetData(const VCD::Preset& a_preset)
    {
        const auto* defaultPresetConfig = VCD::Manager::GetSingleton().GetDefaultPresetConfig(a_preset);
        return defaultPresetConfig ? &defaultPresetConfig->data : nullptr;
    }

    inline const VCD::CollisionData* GetDefaultNPCPresetData(const VCD::Preset& a_preset)
    {
        if (const auto* npcOverride = Settings::GetNPCPresetOverride(a_preset)) {
            return npcOverride;
        }

        return GetDefaultPresetData(a_preset);
    }

    bool PresetCombo(const char* a_label, VCD::Preset& a_preset);

    bool CameraPresetCombo(const char* a_label, VCD::Preset& a_preset);

    void OpenPresetEditor(const VCD::Preset& a_preset);

    void OpenNPCPresetEditor(const VCD::Preset& a_preset, RE::Actor* a_actor);

    bool RefreshPresetEditor();

    void BeginAutoDraw();

    void EndAutoDraw();

    void StopPresetEditorPreview();

    void ClosePresetEditor();

    void UpdateEditedPreset();

    void OpenCreatePresetEditor();

    void CloseCreatePresetEditor();

    void OpenDeletePresetEditor();

    void CloseDeletePresetEditor();

}
