#pragma once

#include "plugin.hpp"
#include "preset.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <unordered_map>

namespace VCD {

    class Manager
    {
    public:

		// Context for an actor's bumper shape and world.
        // Used by drawer or shape dynamics.
        struct ActorBumperContext
        {
            RE::bhkCharacterController* controller{ nullptr };
            RE::bhkWorld* world{ nullptr };
        };

        static Manager& GetSingleton();

        void LoadPresets();
		void ClearLoadedPresets();

        bool SetPreset(const RE::Actor* a_actor, const VCD::Preset& a_preset, const bool& a_log = true);

        bool SetCollisionData(const RE::Actor* a_actor, const VCD::CollisionData& a_data, const VCD::Preset& a_anchorPreset, const char* a_name, const bool& a_log = true);

        bool AreAllPresetsLoaded() const;

        size_t GetLoadedPresetCount() const;

        const std::array<PresetConfig, kPresetCount>& GetPresetConfigs() const noexcept;

        const PresetConfig* GetPresetConfig(const VCD::Preset& a_preset) const;

        PresetConfig* GetPresetConfig(const VCD::Preset& a_preset);

        const PresetConfig* GetDefaultPresetConfig(const VCD::Preset& a_preset) const;

        bool RestorePresetDefault(const VCD::Preset& a_preset);

        RE::hkpCapsuleShape* FindWorldCharacterBumperShape(RE::bhkCharacterController* a_controller) const;

        bool GetActorBumperContext(const RE::Actor* a_actor, ActorBumperContext& a_context, const bool& a_log = true) const;

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

        // Track the state of the bumper anchor for an actor.
        // Crucial for avoiding messing up the bumper center.
        struct BumperAnchorState
        {
            Preset preset{ Preset::kVanillaLike };
            float centerZ{ 0.0F };
            bool valid{ false };
        };

        Manager();

        RE::hkpCapsuleShape* FindCharacterBumperShape(RE::hkpShape* a_shape, const RE::hkpShapeKey& a_key) const;

        // Some unique characters like Werewolf might have multiple capsule shapes, 
        // so we need to find the largest one in the hierarchy as a fallback.
        void FindLargestCapsuleShape(RE::hkpShape* a_shape, CapsuleCandidate& a_candidate) const;

        bool IsCharacterBumperShape(const RE::hkpShape* a_shape, const RE::hkpShapeKey& a_key) const;

        bool IsCharacterBumperMaterial(const RE::MATERIAL_ID& a_materialID) const;

        static LookupFailureState& GetLookupFailureState();

        void LogCharacterBumperFailure(const char* a_reason) const;

        void LogCharacterBumperFailure(const char* a_reason, const std::uint32_t& a_shapeType) const;

        void ClearCharacterBumperFailure() const;

        std::array<PresetConfig, kPresetCount> presetConfigs;
        std::array<PresetConfig, kPresetCount> defaultPresetConfigs;

		// Each actor including player should have their own
        // anchor state to avoid messing up the bumper center.
        std::unordered_map<RE::FormID, BumperAnchorState> bumperAnchorStates;
    };

}
