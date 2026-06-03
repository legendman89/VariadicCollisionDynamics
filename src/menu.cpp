#include "menu.hpp"
#include "preset.hpp"
#include "manager.hpp"

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
            auto manager = VCD::Manager::GetSingleton();
            manager.SetPreset(player, currentPreset);
        }

        if (ImGuiMCP::Button("1.Personal Space"))
        {
            currentPreset = Preset::kPersonalSpace;
            auto manager = VCD::Manager::GetSingleton();
            manager.SetPreset(player, currentPreset);
        }

        if (ImGuiMCP::Button("2.Compact"))
        {
            currentPreset = Preset::kCompact;
            auto manager = VCD::Manager::GetSingleton();
            manager.SetPreset(player, currentPreset);
        }

        if (ImGuiMCP::Button("3.Bulky"))
        {
            currentPreset = Preset::kBulky;
            auto manager = VCD::Manager::GetSingleton();
            manager.SetPreset(player, currentPreset);
        }

        static float radius = 0.0f;

        if (ImGuiMCP::SliderFloat("Capsule Radius", &radius, -100.f, 100.f))
        {
            auto& mesh =
                VCD::Manager::GetSingleton()
                .GetPresetMeshes()[VCD::ToUnderlying(Preset::kBulky)];

            auto* sp = mesh.spCollisionObject.get();
            if (!sp || !sp->body)
                return;

            auto* worldObj =
                static_cast<RE::hkpWorldObject*>(sp->body->referencedObject.get());

            auto* shape = const_cast<RE::hkpShape*>(worldObj->collidable.shape);

            if (!shape || shape->type != RE::hkpShapeType::kCapsule)
                return;

            auto* capsule =
                static_cast<RE::hkpCapsuleShape*>(shape);

            
            capsule->radius = radius;
        }
 
        ImGuiMCP::Separator();
        ImGuiMCP::Text("Active Preset: %d", static_cast<int>(currentPreset));
    }
}