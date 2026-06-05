#include "menu.hpp"
#include "preset.hpp"
#include "manager.hpp"
#include "logger.hpp"
#include "globals.hpp"

namespace UI {

    void Register() {
        if (!SKSEMenuFramework::IsInstalled()) return;

        // call whatever you wish
        SKSEMenuFramework::SetSection("VCD");

        SKSEMenuFramework::AddSectionItem("Debug", RenderDebugMenu);
    }

    // so im still not sure if we need to set the breast meshes aswell or not so this may change drastically bc idek if its correct
    // also planned to implement draw collision after
    void __stdcall RenderDebugMenu() {
        using namespace VCD;

        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player)
            return;

        static Preset currentPreset = Preset::kVanillaLike;

        if (ImGuiMCP::Button("0.Vanilla Like"))
        {
            currentPreset = Preset::kVanillaLike;
            auto& manager = VCD::Manager::GetSingleton();
            logger::info("Debug menu SetPreset result: {}", manager.SetPreset(player, currentPreset));
        }

        if (ImGuiMCP::Button("1.Personal Space"))
        {
            currentPreset = Preset::kPersonalSpace;
            auto& manager = VCD::Manager::GetSingleton();
            logger::info("Debug menu SetPreset result: {}", manager.SetPreset(player, currentPreset));
        }

        if (ImGuiMCP::Button("2.Compact"))
        {
            currentPreset = Preset::kCompact;
            auto& manager = VCD::Manager::GetSingleton();
            logger::info("Debug menu SetPreset result: {}", manager.SetPreset(player, currentPreset));
        }

        if (ImGuiMCP::Button("3.Bulky"))
        {
            currentPreset = Preset::kBulky;
            auto& manager = VCD::Manager::GetSingleton();
            logger::info("Debug menu SetPreset result: {}", manager.SetPreset(player, currentPreset));
        }

        ImGuiMCP::Checkbox("Draw Bumper", &globals::bDrawCharacterBumper);

        auto& manager = VCD::Manager::GetSingleton();
        auto* selectedShape = manager.GetPresetShape(currentPreset);
        static Preset sliderPreset = Preset::kVanillaLike;
        static float radius = 0.0f;

        if (selectedShape && (sliderPreset != currentPreset || radius <= 0.0f)) {
            sliderPreset = currentPreset;
            radius = selectedShape->radius;
        }

        //legendman I dont think this is correct I was told radius is actually vector 4 under the hood even though commonlib says float? 
        // anywho this was only for testing can be removed if needed
        if (selectedShape && ImGuiMCP::SliderFloat("Capsule Radius", &radius, 0.0f, 1.0f))
        {
            selectedShape->radius = radius;
            manager.SetPreset(player, currentPreset);
        }
 
        ImGuiMCP::Separator();
        ImGuiMCP::Text("Active Preset: %d", static_cast<int>(currentPreset));
    }
}
