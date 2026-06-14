#pragma once

#include "menu.hpp"
#include "color.hpp"
#include "logger.hpp"
#include "button.hpp"
#include "manager.hpp"
#include "dynamics.hpp"
#include "settings.hpp"

namespace UI {

    struct PresetEditorState
    {
        bool open{ false };
        bool preview{ false };
        bool autoEnabledDraw{ false };
        bool previousDrawCollision{ false };

        VCD::Preset preset{ VCD::Preset::kVanillaLike };
        VCD::CollisionData defaults{};
        VCD::CollisionData current{};
    };

    inline PresetEditorState& GetPresetEditorState()
    {
        static PresetEditorState state{};
        return state;
    }

    inline void PreviewPreset(const VCD::Preset& a_preset)
    {
        if (const auto* player = RE::PlayerCharacter::GetSingleton()) {
            logger::info("Preset preview result: {}", VCD::Manager::GetSingleton().SetPreset(player, a_preset));
        }
    }

    bool PresetCombo(const char* a_label, VCD::Preset& a_preset);

    void OpenPresetEditor(const VCD::Preset& a_preset);

    bool RefreshPresetEditor();

    void BeginAutoDraw();

    void EndAutoDraw();

    void StopPresetEditorPreview();

    void ClosePresetEditor();

    void UpdateEditedPreset();

}
