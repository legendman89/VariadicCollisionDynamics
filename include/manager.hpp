#pragma once

#include "plugin.hpp"
#include "preset.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace VCD {

    class Manager
    {
    public:

		// Context for an actor's bumper shape and world.
        // Used by drawer or shape dynamics.
        struct ActorBumperContext
        {
            RE::bhkCharProxyController* controller{ nullptr };
            RE::bhkWorld* world{ nullptr };
        };

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

        RE::hkpCapsuleShape* FindWorldCharacterBumperShape(RE::bhkCharProxyController* a_controller) const;

        bool GetActorBumperContext(const RE::Actor* a_actor, ActorBumperContext& a_context) const;

    private:

		// Beast might have different shapes, so we need to 
        // find the largest capsule shape in the hierarchy.
        struct CapsuleCandidate
        {
            RE::hkpCapsuleShape* capsule{ nullptr };
            float height{ 0.0F };
        };

		// Track the reason for a failed lookup of a character bumper shape.
        struct LookupFailureState
        {
            const char* reason{ nullptr };
            std::uint32_t shapeType{ std::numeric_limits<std::uint32_t>::max() };
        };

        Manager();

        RE::hkpCapsuleShape* FindCharacterBumperShape(RE::hkpShape* a_shape, const RE::hkpShapeKey& a_key) const;

        void FindLargestCapsuleShape(RE::hkpShape* a_shape, CapsuleCandidate& a_candidate) const;

        bool IsCharacterBumperShape(const RE::hkpShape* a_shape, const RE::hkpShapeKey& a_key) const;

        bool IsCharacterBumperMaterial(const RE::MATERIAL_ID& a_materialID) const;

        static LookupFailureState& GetLookupFailureState();

        void LogCharacterBumperFailure(const char* a_reason) const;

        void LogCharacterBumperFailure(const char* a_reason, const std::uint32_t& a_shapeType) const;

        void ClearCharacterBumperFailure() const;

        std::array<PresetConfig, 4> presetConfigs;
        std::array<PresetConfig, 4> defaultPresetConfigs;
    };

}
