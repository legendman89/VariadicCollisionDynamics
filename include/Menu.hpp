#pragma once

#include "ui.hpp"
#include "color.hpp"
#include "icons.hpp"

#include <algorithm>
#include <string>

namespace UI {

    constexpr auto kPresetActionButtonWidth = 220.0F;
    constexpr auto kPresetActionButtonHeight = 50.0F;
    constexpr auto kFixedSliderWidth = 260.0F;

    inline void SolidBackground(const GUI::ImGuiCol& bgType) {
        auto bg = GUI::GetStyleColorVec4(bgType);
        GUI::ImVec4 newBg = *bg;
        newBg.w = 1.0f; // force opaque
        GUI::PushStyleColor(bgType, newBg);
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

    inline float IconCTAButtonWidth(const char* a_label)
    {
        const auto label = "      " + std::string(a_label);
        GUI::ImVec2 labelSize{};
        GUI::CalcTextSize(&labelSize, label.c_str(), nullptr, false, -1.0F);
        return labelSize.x + 28.0F;
    }

    inline float IconCTAButtonHeight()
    {
        const auto* style = GUI::GetStyle();
        return GUI::GetFontSize() + ((style ? style->FramePadding.y : 6.0F) * 2.0F);
    }

    inline float PresetActionButtonWidth(const char* a_label)
    {
        const auto label = "        " + std::string(a_label);
        GUI::ImVec2 labelSize{};
        GUI::CalcTextSize(&labelSize, label.c_str(), nullptr, false, -1.0F);
        return std::max(kPresetActionButtonWidth, labelSize.x + 28.0F);
    }

    inline void CenterNextWindow(const GUI::ImGuiCond& a_cond = GUI::ImGuiCond_Appearing)
    {
        if (const auto* io = GUI::GetIO()) {
            GUI::SetNextWindowPos(GUI::ImVec2{ io->DisplaySize.x * 0.5F, io->DisplaySize.y * 0.5F }, a_cond, GUI::ImVec2{ 0.5F, 0.5F });
        }
    }

    inline void DrawLine(const GUI::ImVec2& a_start, const GUI::ImVec2& a_end, const GUI::ImVec4& a_color, const float& a_thickness = 1.0F)
    {
        GUI::ImDrawListManager::AddLine(
            GUI::GetWindowDrawList(),
            a_start,
            a_end,
            GUI::GetColorU32(a_color),
            a_thickness
        );
    }

    template <class LeftRenderer, class RightRenderer>
    inline void RenderActionBar(
        const float& a_leftWidth,
        const float& a_leftHeight,
        const float& a_rightWidth,
        const float& a_rightHeight,
        LeftRenderer a_leftRenderer,
        RightRenderer a_rightRenderer)
    {
        GUI::Spacing();

        const auto* style = GUI::GetStyle();
        const auto spacing = style ? style->ItemSpacing.x : 8.0F;
        const bool hasLeft = a_leftWidth > 0.0F && a_leftHeight > 0.0F;
        const bool hasRight = a_rightWidth > 0.0F && a_rightHeight > 0.0F;

        GUI::ImVec2 start{};
        GUI::ImVec2 available{};
        GUI::ImVec2 screenStart{};
        GUI::GetCursorPos(&start);
        GUI::GetContentRegionAvail(&available);
        GUI::GetCursorScreenPos(&screenStart);

        const auto singleRowWidth = (hasLeft ? a_leftWidth : 0.0F) + (hasLeft && hasRight ? spacing : 0.0F) + (hasRight ? a_rightWidth : 0.0F);
        const auto singleRow = singleRowWidth <= available.x;
        const auto actionBarHeight = singleRow ? 
                std::max(hasLeft ? a_leftHeight : 0.0F, hasRight ? a_rightHeight : 0.0F) : 
                (hasLeft ? a_leftHeight + spacing : 0.0F) + (hasRight ? a_rightHeight : 0.0F);
        GUI::ImVec2 afterLeft{};
        if (hasLeft) {
            a_leftRenderer();
            GUI::GetCursorPos(&afterLeft);
        }

        if (hasRight) {
            const auto rightX = singleRow ? start.x + std::max(0.0F, available.x - a_rightWidth) : start.x;
            const auto rightY = singleRow || !hasLeft ? start.y : afterLeft.y + spacing;

            GUI::SetCursorPos(GUI::ImVec2{ rightX, rightY });
            a_rightRenderer();
        }

        GUI::SetCursorPos(GUI::ImVec2{ start.x, start.y + actionBarHeight });
        GUI::Spacing();
        GUI::Spacing();

        GUI::ImVec2 separatorStart{};
        GUI::GetCursorScreenPos(&separatorStart);
        DrawLine(
            GUI::ImVec2{ screenStart.x, separatorStart.y },
            GUI::ImVec2{ screenStart.x + available.x, separatorStart.y },
            Color::kActionBarRule,
            2.0F
        );
        GUI::Dummy(GUI::ImVec2{ 0.0F, 2.0F });
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

    void Register();

}
