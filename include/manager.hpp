#pragma once

#include "plugin.hpp"
#include "preset.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <functional>
#include <limits>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace VCD {

    struct PresetKeyHash
    {
        using is_transparent = void;

        size_t operator()(std::string_view a_key) const noexcept
        {
            return std::hash<std::string_view>{}(a_key);
        }

        size_t operator()(const std::string& a_key) const noexcept
        {
            return operator()(std::string_view(a_key));
        }
    };

    class Manager
    {
    public:

        inline static Manager& GetSingleton()
        {
            static Manager singleton;
            return singleton;
        }

        inline const std::vector<PresetConfig>& GetPresetConfigs() const noexcept
        {
            return presetConfigs;
        }

        inline const PresetConfig* GetPresetConfig(const Preset& a_preset) const
        {
            const auto index = static_cast<size_t>(ToUnderlying(a_preset));
            if (index >= presetConfigs.size()) {
                return nullptr;
            }

            return &presetConfigs[index];
        }

        inline const PresetConfig* GetPresetConfig(std::string_view a_key) const
        {
            const auto it = presetIndices.find(a_key);
            return it != presetIndices.end() && it->second < presetConfigs.size() ? &presetConfigs[it->second] : nullptr;
        }

        inline PresetConfig* GetPresetConfig(const Preset& a_preset)
        {
            return const_cast<PresetConfig*>(static_cast<const Manager*>(this)->GetPresetConfig(a_preset));
        }

        inline PresetConfig* GetPresetConfig(std::string_view a_key)
        {
            return const_cast<PresetConfig*>(static_cast<const Manager*>(this)->GetPresetConfig(a_key));
        }

        inline const PresetConfig* GetDefaultPresetConfig(const Preset& a_preset) const
        {
            const auto index = static_cast<size_t>(ToUnderlying(a_preset));
            if (index >= defaultPresetConfigs.size()) {
                return nullptr;
            }

            return &defaultPresetConfigs[index];
        }

        // Context for an actor's bumper shape and world.
        // Used by drawer or shape dynamics.
        struct ActorBumperContext
        {
            RE::bhkCharacterController* controller{ nullptr };
            RE::bhkWorld* world{ nullptr };
        };

        bool GetActorBumperContext(const RE::Actor* a_actor, ActorBumperContext& a_context, const bool& a_log = true) const;

        RE::hkpCapsuleShape* FindWorldCharacterBumperShape(RE::bhkCharacterController* a_controller) const;

        void LoadPresets();

        void ClearLoadedPresets();

        void ClearRuntimeState();

        void ClearActorRuntimeState(const RE::FormID& a_formID);

        void ClearActorTransientState(const RE::FormID& a_formID);

        bool CreatePreset(const std::string& a_name, const CollisionData& a_data, Preset& a_preset, std::string& a_error);

        bool DeletePreset(const Preset& a_preset, std::string& a_key, std::string& a_error);

        bool SetPreset(const RE::Actor* a_actor, const Preset& a_preset, const PoseFlags& a_poseFlags, const bool& a_log);

        bool SetCollisionData(const RE::Actor* a_actor, const CollisionData& a_data, const Preset& a_anchorPreset, const char* a_name, const PoseFlags& a_poseFlags, const bool& a_log);

        bool FixSittingPose(const RE::Actor* a_actor, const PoseFlags& a_poseFlags, const bool& a_log = false);

        bool FixSneakingPose(const RE::Actor* a_actor, const PoseFlags& a_poseFlags, const bool& a_log = false);

        float GetStandingCapsuleHeight(const RE::Actor* a_actor) const;

        size_t GetLoadedPresetCount() const;

        bool RestorePresetDefault(const Preset& a_preset);

        RE::hkpSphereShape* GetCameraPhantomShape(RE::bhkSimpleShapePhantom* bhkPhantom);

        bool IsCameraPreset(VCD::Preset a_preset) const;

    private:

        Manager();

        void RebuildPresetIndex();

		// Beast might have different shapes, so we need to 
        // find the largest capsule shape in the hierarchy.
        struct CapsuleCandidate
        {
            RE::hkpCapsuleShape* capsule{ nullptr };
            RE::hkpListShape::ChildInfo* childInfo{ nullptr };
            float height{ 0.0F };
        };

        // Tracks Bumper shape when sweeping the tree.
        struct CharacterBumperShape
        {
            RE::hkpShape* rootShape{ nullptr };
            RE::hkpCapsuleShape* capsule{ nullptr };
            RE::hkpListShape::ChildInfo* childInfo{ nullptr };
        };

        // Tracks Convex shape when sweeping the tree.
        struct ConvexShapeData
        {
            RE::hkpShape* rootShape{ nullptr };
            RE::hkpListShape* listShape{ nullptr };
            RE::hkpListShape::ChildInfo* childInfo{ nullptr };
            RE::hkpConvexVerticesShape* convexShape{ nullptr };
        };

        // State of Convex data.
        struct ConvexShapeState
        {
            std::vector<RE::hkVector4> originalVertices{};
            const RE::bhkCharacterController* controller{ nullptr };
            const RE::hkpConvexVerticesShape* currentShape{ nullptr };
            float originalRadius{ 0.0F };
            bool valid{ false };
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
            Preset preset{ Preset::kVanilla };
            float centerZ{ 0.0F };
            bool valid{ false };
            bool fromSitting{ false };
        };

        // Track the state of an actor for adjusting the bumper shape.
        struct LastActorState {
            float standingPoint1Z{ 0.0F };
            float standingPoint2Z{ 0.0F };
            float standingRadius{ 0.0F };
            RE::NiPoint3 standingTranslation{};
			bool hasStandingCapsule{ false };
            bool hasStandingTranslation{ false };
            bool sittingPoseApplied{ false };
            bool sneakingPoseApplied{ false };
        };

        inline float GetCapsuleHeight(const float& a_point1Z, const float& a_point2Z, const float& a_radius) const
        {
            return std::abs(a_point1Z - a_point2Z) + 2.0F * a_radius;
        }

        inline static LookupFailureState& GetLookupFailureState()
        {
            static LookupFailureState state{};
            return state;
        }

        inline void CacheStandingCapsule(LastActorState& a_state, const float& a_point1Z, const float& a_point2Z, const float& a_radius)
        {
            a_state.standingPoint1Z = a_point1Z;
            a_state.standingPoint2Z = a_point2Z;
            a_state.standingRadius = a_radius;
            a_state.hasStandingCapsule = true;
        }

        inline float ApplySittingCapsule(LastActorState& a_state, float& a_point1Z, float& a_point2Z, const float& a_radius, const PoseFlags& a_poseFlags, const float& a_scale)
        {
            const auto sittingHeight = GetCapsuleHeight(a_state.standingPoint1Z, a_state.standingPoint2Z, a_state.standingRadius) * a_scale;
            const auto sittingCenterlineHeight = std::max(0.0F, sittingHeight - 2.0F * a_state.standingRadius);

            if (a_state.standingPoint1Z >= a_state.standingPoint2Z) {
                a_point1Z = a_poseFlags.isChildSittingOnKnees ? 0.0F : sittingCenterlineHeight;
                a_point2Z = a_poseFlags.isChildSittingOnKnees ? a_state.standingPoint2Z : 0.0F;
            }
            else {
                a_point1Z = a_poseFlags.isChildSittingOnKnees ? a_state.standingPoint1Z : 0.0F;
                a_point2Z = a_poseFlags.isChildSittingOnKnees ? 0.0F : sittingCenterlineHeight;
            }

            return GetCapsuleHeight(a_point1Z, a_point2Z, a_radius);
        }

        inline void ApplySneakingCapsule(LastActorState& a_state, float& a_point1Z, float& a_point2Z, const float& a_scale)
        {
            const auto sneakingHeight = GetCapsuleHeight(a_state.standingPoint1Z, a_state.standingPoint2Z, a_state.standingRadius) * a_scale;
            const auto sneakingCenterlineHeight = std::max(0.0F, sneakingHeight - 2.0F * a_state.standingRadius);

            if (a_state.standingPoint1Z >= a_state.standingPoint2Z) {
                a_point2Z = a_state.standingPoint2Z;
                a_point1Z = a_point2Z + sneakingCenterlineHeight;
            }
            else {
                a_point1Z = a_state.standingPoint1Z;
                a_point2Z = a_point1Z + sneakingCenterlineHeight;
            }
        }

        inline void ApplyCapsulePoseHeight(RE::hkpCapsuleShape* a_shape, const float& a_radius, const float& a_point1Z, const float& a_point2Z)
        {
            const auto vertexA = ToNiPoint3(a_shape->vertexA);
            const auto vertexB = ToNiPoint3(a_shape->vertexB);

            a_shape->radius = a_radius;
            a_shape->vertexA = ToHkVector(RE::NiPoint3(vertexA.x, vertexA.y, a_point1Z));
            a_shape->vertexB = ToHkVector(RE::NiPoint3(vertexB.x, vertexB.y, a_point2Z));
        }

        // Recompute convex vertices based on capsule shape.
        bool SetConvexShape(const RE::Actor* a_actor, RE::bhkCharacterController* a_controller, const float& a_radius, const float& a_point1Z, const float& a_point2Z, const RE::NiPoint3& a_translation, const char* a_name, const bool& a_log);

        RE::hkpShape* GetControllerRootShape(RE::bhkCharacterController* a_controller) const;

        bool FindWorldCharacterBumperShapeData(RE::bhkCharacterController* a_controller, CharacterBumperShape& a_bumper) const;

        bool FindCharacterBumperShape(RE::hkpShape* a_shape, const RE::hkpShapeKey& a_key, CharacterBumperShape& a_bumper) const;

        bool FindControllerConvexShape(RE::bhkCharacterController* a_controller, ConvexShapeData& a_convex) const;

        // Some unique characters like Werewolf might have multiple capsule shapes, 
        // so we need to find the largest one in the hierarchy as a fallback.
        void FindLargestCapsuleShape(RE::hkpShape* a_shape, CapsuleCandidate& a_candidate, RE::hkpListShape::ChildInfo* a_childInfo = nullptr) const;

        bool IsCharacterBumperShape(const RE::hkpShape* a_shape, const RE::hkpShapeKey& a_key) const;

        bool CacheConvexShapeState(const RE::FormID& a_formID, const RE::bhkCharacterController* a_controller, const RE::hkpConvexVerticesShape* a_shape, ConvexShapeState& a_state);

        bool ReplaceControllerConvexShape(RE::bhkCharacterController* a_controller, ConvexShapeData& a_convex, RE::hkpConvexVerticesShape* a_newShape) const;

        // For debugging.
        void LogCharacterBumperFailure(const char* a_reason) const;

        void LogCharacterBumperFailure(const char* a_reason, const std::uint32_t& a_shapeType) const;

        void ClearCharacterBumperFailure() const;

        // Presets containers.
        std::vector<PresetConfig> presetConfigs;
        std::vector<PresetConfig> defaultPresetConfigs;

        // This allows fast lookup given a string as key.
        // Thanks to the hashing function PresetKeyHash.
        // Implicitly it maps key to indeger index which
        // is used later to index presetConfigs vector.
        std::unordered_map<std::string, size_t, PresetKeyHash, std::equal_to<>> presetIndices;

		// Each actor including player should have their own state to avoid messing up each other.
        std::unordered_map<RE::FormID, BumperAnchorState> bumperAnchorStates;
        std::unordered_map<RE::FormID, ConvexShapeState> convexShapeStates;
        std::unordered_map<RE::FormID, LastActorState> actorStates;
    };

    inline const char* PresetKey(const Preset& a_preset)
    {
        const auto* config = Manager::GetSingleton().GetPresetConfig(a_preset);
        return config ? config->key.c_str() : kPresetKeys[0];
    }

    inline const char* PresetName(const Preset& a_preset)
    {
        const auto* config = Manager::GetSingleton().GetPresetConfig(a_preset);
        return config ? config->name.c_str() : "Unknown";
    }

    inline bool IsCharacterBumperMaterial(const RE::MATERIAL_ID& a_materialID)
    {
        if (a_materialID == RE::MATERIAL_ID::kCharacterBumper) {
            return true;
        }

        return a_materialID == RE::MATERIAL_ID::kTrap;
    }

}
