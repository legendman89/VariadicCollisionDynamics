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

        void LoadPresets();
		void ClearLoadedPresets();

        bool SetPreset(const RE::Actor* a_actor, const VCD::Preset& a_preset);

        bool AreAllPresetsLoaded() const;

        std::size_t GetLoadedPresetCount() const;

        const std::array<PresetConfig, 4>& GetPresetConfigs() const noexcept;

        const PresetConfig* GetPresetConfig(const VCD::Preset& a_preset) const;

        PresetConfig* GetPresetConfig(const VCD::Preset& a_preset);

        const PresetConfig* GetDefaultPresetConfig(const VCD::Preset& a_preset) const;

        bool RestorePresetDefault(const VCD::Preset& a_preset);

        void DrawPlayerBumper(); 

        RE::hkpCapsuleShape* FindWorldCharacterBumperShape(RE::bhkCharProxyController* a_controller) const;

    private:

        Manager();

        RE::hkpCapsuleShape* FindCharacterBumperShape(RE::hkpShape* a_shape, const RE::hkpShapeKey& a_key) const;

        bool IsCharacterBumperShape(const RE::hkpShape* a_shape, const RE::hkpShapeKey& a_key) const;

        std::array<PresetConfig, 4> presetConfigs;
        std::array<PresetConfig, 4> defaultPresetConfigs;
    };

}
