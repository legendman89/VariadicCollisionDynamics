#pragma once

#include "color.hpp"

namespace UI {

    inline bool CTAButton(const char* a_label, const bool& a_enabled)
    {
        GUI::PushStyleVar(GUI::ImGuiStyleVar_FrameRounding, 6.0F);
        GUI::PushStyleVar(GUI::ImGuiStyleVar_FramePadding, GUI::ImVec2{ 14.0F, 6.0F });
        GUI::PushStyleColor(GUI::ImGuiCol_Button, a_enabled ? Color::kCTAOnBG : Color::kCTAOffBG);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonHovered, a_enabled ? Color::kCTAOnHover : Color::kCTAOffHover);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonActive, a_enabled ? Color::kCTAOnActive : Color::kCTAOffActive);
        GUI::PushStyleColor(GUI::ImGuiCol_Text, a_enabled ? Color::kCTAOnText : Color::kCTAOffText);

        if (!a_enabled) {
            GUI::BeginDisabled();
        }

        const bool clicked = GUI::Button(a_label);

        if (!a_enabled) {
            GUI::EndDisabled();
        }

        GUI::PopStyleColor(4);
        GUI::PopStyleVar(2);
        return clicked && a_enabled;
    }

    inline void Tooltip(const char* a_text)
    {
        GUI::SetItemTooltip("%s", a_text);
    }

}
