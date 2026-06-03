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

        //works but player falls through floor halfway not sure why yet
        bool SetPreset(RE::Actor* a_actor, VCD::Preset a_preset);

        bool SetCollisionShape(RE::bhkCharProxyController* a_controller,
            const RE::hkpShape* a_shape);

        bool AreAllPresetMeshesLoaded() const;
        std::size_t GetLoadedPresetCount() const;
        const std::array<PresetMesh, 4>& GetPresetMeshes() const noexcept;

    private:

        Manager();

        bool LoadPresetMesh(PresetMesh& a_mesh);

        RE::NiAVObject* FindCharacterBumper(RE::NiNode* a_root) const;

        std::array<PresetMesh, 4> presetMeshes;
    };

}
