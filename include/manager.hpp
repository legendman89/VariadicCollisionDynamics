#pragma once

#include "plugin.hpp"
#include "preset.hpp"

#include <array>
#include <cstddef>

namespace VCD {

    class Manager
    {
    public:

        static Manager& GetSingleton();

        void LoadPresetMeshes();
		void ClearLoadedMeshes();

        bool SetPreset(const RE::Actor* a_actor, const VCD::Preset& a_preset);

        bool SetCollisionShape(RE::bhkCharProxyController* a_controller, const RE::hkpShape* a_shape);

        bool AreAllPresetMeshesLoaded() const;
        std::size_t GetLoadedPresetCount() const;
        const std::array<PresetMesh, 4>& GetPresetMeshes() const noexcept;
        const PresetMesh* GetPresetMesh(const VCD::Preset& a_preset) const;
        RE::hkpCapsuleShape* GetPresetShape(const VCD::Preset& a_preset);
        const RE::hkpCapsuleShape* GetPresetShape(const VCD::Preset& a_preset) const;
        void DrawPlayerBumper(); 
        RE::hkpCapsuleShape* FindWorldCharacterBumperShape(RE::bhkCharProxyController* a_controller) const;
    private:

        Manager();

        bool LoadPresetMesh(PresetMesh& a_mesh);

        RE::NiAVObject* FindCharacterBumper(RE::NiNode* a_root) const;
      
        RE::hkpCapsuleShape* FindCharacterBumperShape(RE::hkpShape* a_shape, const RE::hkpShapeKey& a_key) const;
        bool IsCharacterBumperShape(const RE::hkpShape* a_shape, const RE::hkpShapeKey& a_key) const;

        std::array<PresetMesh, 4> presetMeshes;
    };

}
