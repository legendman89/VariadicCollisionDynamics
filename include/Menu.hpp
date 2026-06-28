#pragma once

#include "ui.hpp"
#include "color.hpp"
#include "icons.hpp"

#include <string>

namespace UI {

    void Register();

    inline void SolidBackground(const GUI::ImGuiCol& bgType) {
        auto bg = GUI::GetStyleColorVec4(bgType);
        GUI::ImVec4 newBg = *bg;
        newBg.w = 1.0f; // force opaque
        GUI::PushStyleColor(bgType, newBg);
    }

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

    inline bool IconCTAButton(const char* a_label, const bool& a_enabled, const unsigned a_icon)
    {
        const auto iconText = FontAwesome::UnicodeToUtf8(a_icon);

        GUI::PushStyleVar(GUI::ImGuiStyleVar_FrameRounding, 6.0F);
        GUI::PushStyleVar(GUI::ImGuiStyleVar_FramePadding, GUI::ImVec2{ 14.0F, 6.0F });
        GUI::PushStyleColor(GUI::ImGuiCol_Button, a_enabled ? Color::kCTAOnBG : Color::kCTAOffBG);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonHovered, a_enabled ? Color::kCTAOnHover : Color::kCTAOffHover);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonActive, a_enabled ? Color::kCTAOnActive : Color::kCTAOffActive);
        GUI::PushStyleColor(GUI::ImGuiCol_Text, a_enabled ? Color::kCTAOnText : Color::kCTAOffText);

        if (!a_enabled) {
            GUI::BeginDisabled();
        }

        const bool clicked = GUI::Button(("      " + std::string(a_label)).c_str());

        if (!a_enabled) {
            GUI::EndDisabled();
        }

        GUI::ImVec2 buttonMin{};
        GUI::ImVec2 buttonMax{};

        GUI::GetItemRectMin(&buttonMin);
        GUI::GetItemRectMax(&buttonMax);

        FontAwesome::PushSolid();

        const auto* iconFont = GUI::GetFont();
        const auto iconFontSize = GUI::GetFontSize();
        const auto iconColor = GUI::GetColorU32(a_enabled ? Color::kCTAOnText : Color::kCTAOffText);
        GUI::ImVec2 iconSize{};
        GUI::CalcTextSize(&iconSize, iconText.c_str(), nullptr, false, -1.0F);
        FontAwesome::Pop();

        GUI::ImDrawListManager::AddText(
            GUI::GetWindowDrawList(),
            iconFont,
            iconFontSize,
            GUI::ImVec2{ buttonMin.x + 13.0F, buttonMin.y + ((buttonMax.y - buttonMin.y - iconSize.y) * 0.5F) },
            iconColor,
            iconText.c_str()
        );

        GUI::PopStyleColor(4);
        GUI::PopStyleVar(2);
        return clicked && a_enabled;
    }

    inline bool EditTextButton(const char* a_label)
    {
        static const auto editText = FontAwesome::UnicodeToUtf8(Icons::kEdit);

        GUI::PushStyleColor(GUI::ImGuiCol_ButtonHovered, Color::kEditHover);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonActive, Color::kEditActive);

        const bool clicked = GUI::Button(("       " + std::string(a_label)).c_str());

        GUI::ImVec2 buttonMin{};
        GUI::ImVec2 buttonMax{};

        GUI::GetItemRectMin(&buttonMin);
        GUI::GetItemRectMax(&buttonMax);

        FontAwesome::PushRegular();

        const auto* iconFont = GUI::GetFont();
        const auto iconFontSize = GUI::GetFontSize();
        const auto iconColor = GUI::GetColorU32(Color::kCreatePresetText);
        GUI::ImVec2 iconSize{};
        GUI::CalcTextSize(&iconSize, editText.c_str(), nullptr, false, -1.0F);
        FontAwesome::Pop();

        GUI::ImDrawListManager::AddText(
            GUI::GetWindowDrawList(),
            iconFont,
            iconFontSize,
            GUI::ImVec2{ buttonMin.x + 13.0F, buttonMin.y + ((buttonMax.y - buttonMin.y - iconSize.y) * 0.5F) },
            iconColor,
            editText.c_str()
        );

        GUI::PopStyleColor(2);
        return clicked;
    }

    inline bool EditButton(const char* a_id)
    {
        static const auto editText = FontAwesome::UnicodeToUtf8(Icons::kEdit);

        GUI::PushStyleVar(GUI::ImGuiStyleVar_FrameBorderSize, 0.0F);
        GUI::PushStyleVar(GUI::ImGuiStyleVar_FramePadding, GUI::ImVec2{ 0.0F, 0.0F });
        GUI::PushStyleColor(GUI::ImGuiCol_Button, Color::kTransparent);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonHovered, Color::kTransparent);
        GUI::PushStyleColor(GUI::ImGuiCol_ButtonActive, Color::kTransparent);
        GUI::PushStyleColor(GUI::ImGuiCol_Text, Color::kCreatePresetText);
        FontAwesome::PushRegular();
        GUI::SetWindowFontScale(1.25F);

        const bool clicked = GUI::Button((editText + "##" + a_id).c_str(), GUI::ImVec2{ 48.0F, 0.0F });

        GUI::SetWindowFontScale(1.0F);
        FontAwesome::Pop();
        GUI::PopStyleColor(4);
        GUI::PopStyleVar(2);
        return clicked;
    }

    inline void Tooltip(const char* a_text)
    {
        GUI::SetItemTooltip("%s", a_text);
    }

    inline void WrappedTooltip(const char* a_text, const float& a_width = 420.0F)
    {
        if (GUI::IsItemHovered()) {
            GUI::BeginTooltip();
            GUI::PushTextWrapPos(a_width);
            GUI::TextUnformatted(a_text);
            GUI::PopTextWrapPos();
            GUI::EndTooltip();
        }
    }

}
