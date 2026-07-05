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
#include <chrono>
#include <cstddef>

namespace UI {

    // Editor debounce time.
    inline constexpr auto kEditorCollisionApplyDebounce = std::chrono::milliseconds(250);

    // Fixed widths.
    inline constexpr auto kFixedDynamicsStateColumnWidth = 210.0F;
    inline constexpr auto kFixedComboWidth = 220.0F;
    inline constexpr auto kFixedPreviewComboWidth = 260.0F;
    inline constexpr auto kFixedActorComboWidth = 270.0F;
    inline constexpr auto kFixedDeletePresetComboWidth = 260.0F;
    inline constexpr auto kFixedCreatePresetInputWidth = 260.0F;
    inline constexpr auto kPresetEditorWindowSize = GUI::ImVec2{ 520.0F, 395.0F };
    inline constexpr auto kCreatePresetEditorWindowSize = GUI::ImVec2{ 480.0F, 450.0F };
    inline constexpr auto kDeletePresetEditorWindowSize = GUI::ImVec2{ 450.0F, 250.0F };

    struct CollisionEditorLimits
    {
        float heightOffset{ 4.0F };
        float radius{ 1.0F };
        float xyOffset{ 30.0F };
    };

    inline constexpr std::array<CollisionEditorLimits, static_cast<size_t>(VCD::Race::CollisionLimitClass::kTotal)> kCollisionEditorLimits
    {
        CollisionEditorLimits{ 0.6F, 1.0F, 30.0F },
        CollisionEditorLimits{ 0.8F, 1.0F, 30.0F },
        CollisionEditorLimits{ 2.5F, 1.5F, 30.0F },
        CollisionEditorLimits{ 2.0F, 1.2F, 30.0F },
        CollisionEditorLimits{ 3.0F, 1.0F, 30.0F },
        CollisionEditorLimits{ 0.0F, 30.0F, 30.0F }
    };

    inline constexpr CollisionEditorLimits GetCollisionEditorLimits(const VCD::Race::CollisionLimitClass& a_limitClass)
    {
        const auto index = static_cast<size_t>(a_limitClass);
        return index < kCollisionEditorLimits.size() ? kCollisionEditorLimits[index] : kCollisionEditorLimits[static_cast<size_t>(VCD::Race::CollisionLimitClass::kDefault)];
    }

    struct PresetEditorState
    {
        bool open{ false };
        bool preview{ false };
        bool npcPreview{ false };
        bool npcGlobal{ false };
        bool camera{ false };
        bool autoEnabledDraw{ false };
        bool previousDrawCollision{ false };
        bool previousDrawNearbyActors{ false };
        bool collisionApplyPending{ false };
        bool collisionApplyAfterRelease{ false };

        RE::ActorHandle previewActor{};
        VCD::Preset preset{ VCD::Preset::kVanilla };
        VCD::CollisionData defaults{};
        VCD::CollisionData current{};
        CollisionEditorLimits limits{};
        std::chrono::steady_clock::time_point collisionApplyAt{};
    };

    struct CreatePresetEditorState
    {
        bool open{ false };
        bool preview{ false };
        std::array<char, 128> name{};
        std::string error{};
        VCD::CollisionData defaults{};
        VCD::CollisionData current{};
        CollisionEditorLimits limits{};
    };

    struct DeletePresetEditorState
    {
        bool open{ false };
        VCD::Preset preset{ VCD::Preset::kVanilla };
        std::string error{};
    };

    struct CollisionSliderResult
    {
        bool changed{ false };
        bool active{ false };
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

    inline void CloseDeletePresetEditor()
    {
        GetDeletePresetEditorState() = {};
    }

    bool PresetCombo(const char* a_label, VCD::Preset& a_preset);

    bool CameraPresetCombo(const char* a_label, VCD::Preset& a_preset);

    void OpenPresetEditor(const VCD::Preset& a_preset);

    void OpenNPCPresetEditor(const VCD::Preset& a_preset, RE::Actor* a_actor);

    bool RefreshPresetEditor();

    void BeginAutoDraw();

    void EndAutoDraw();

    bool StartNPCEditorPreview(RE::Actor* a_actor);

    void StopNPCEditorPreview();

    void StopPresetEditorPreview();

    void ClosePresetEditor();

    void UpdateEditedPreset();

    void OpenCreatePresetEditor();

    void CloseCreatePresetEditor();

    void OpenDeletePresetEditor();

    void RenderPlayerDynamics();

    void RenderNPCDynamics();
    
    void RenderCameraDynamics();

}
