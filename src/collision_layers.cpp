#include "collision_layers.hpp"


namespace CollisionLayers
{
    // Order-insensitive comparison of two collision layer override lists.
    bool IsLayerOverridden(const std::vector<std::string>& a_overrides, const std::string_view a_layer)
    {
        return std::find(a_overrides.begin(), a_overrides.end(), a_layer) != a_overrides.end();
    }

    // We only serialize the difference in layers from vanilla.
    bool GetLayerEnabled(const size_t a_index, const std::vector<std::string>& a_overrides)
    {
        if (a_index >= kCollisionLayerNames.size()) {
            return false;
        }

        constexpr auto charControllerIndex = std::to_underlying(RE::COL_LAYER::kCharController);
        const auto vanillaMask = kVanillaCollisionMasks[charControllerIndex];
        const auto bit = uint64_t{ 1 } << a_index;
        const bool vanillaEnabled = (vanillaMask & bit) != 0;

        return IsLayerOverridden(a_overrides, kCollisionLayerNames[a_index]) ? !vanillaEnabled : vanillaEnabled;
    }

    // The shared default for every actor is the vanilla kCharController mask.
    // Returns true when no layer differs from it.
    bool IsSameAsDefault(const std::vector<std::string>& a_overrides)
    {
        constexpr auto charControllerIndex = std::to_underlying(RE::COL_LAYER::kCharController);
        const auto vanillaMask = kVanillaCollisionMasks[charControllerIndex];

        for (size_t i = 0; i < kCollisionLayerNames.size(); ++i) {
            const bool vanillaEnabled = ((vanillaMask >> i) & 1) != 0;
            if (GetLayerEnabled(i, a_overrides) != vanillaEnabled) {
                return false;
            }
        }

        return true;
    }

    // Overrides are stored as a diff from vanilla, so restoring vanilla means no overrides.
    void RestoreToDefaultLayers(std::vector<std::string>& a_overrides)
    {
        a_overrides.clear();
    }

    RE::Actor* GetCollisionActor(const RE::hkpCollidable& a_collidable)
    {
        auto* ref = RE::TESHavokUtilities::FindCollidableRef(a_collidable);
        return ref ? skyrim_cast<RE::Actor*>(ref) : nullptr;
    }

    bool ShouldRejectNPCCollision(RE::Actor* a_actor, RE::COL_LAYER a_otherLayer)
    {
        if (!a_actor) {
            return false;
        }

        const auto index = std::to_underlying(a_otherLayer);

        if (index >= CollisionLayers::kCollisionLayerNames.size()) {
            return false;
        }

        auto& manager = VCD::Manager::GetSingleton();
        auto* actorState = Dynamics::FindNPCPresetState(a_actor->GetFormID());

        VCD::Preset preset;

        if (actorState) {
            preset = actorState->preset;
        }
        else if (a_actor->HasKeywordString("ActorTypeGhost")) {
            preset = a_actor->IsInCombat() ? VCD::Preset::kGhostCombat : VCD::Preset::kGhostNeutral;
        }
        else {
            return false;
        }

        const auto* presetConfig = manager.GetPresetConfig(preset);

        if (!presetConfig) {
            return false;
        }

        const auto& layerName = CollisionLayers::kCollisionLayerNames[index];

        const auto* actorLayers = Settings::GetNPCActorPresetCollisionLayers(a_actor->GetFormID(), preset);
        const auto* raceLayers = VCD::Race::GetRaceCollisionLayers(a_actor->GetRace());

        const auto& collisionLayers = actorLayers ? *actorLayers : raceLayers ? *raceLayers : presetConfig->collisionLayers;

        const bool overridden = CollisionLayers::IsLayerOverridden(collisionLayers, layerName);

        logger::debug(
            "[NPC Collision] {} [{:08X}] | Preset: {} | Other layer: {} | Override: {}",
            a_actor->GetDisplayFullName(),
            a_actor->GetFormID(),
            VCD::PresetName(preset),
            layerName,
            overridden);

        return overridden;
    }

    RE::TESObjectWEAP* GetMeleeHitWeapon(RE::Actor* a_attacker, char a_left)
    {
        if (!a_attacker) {
            return nullptr;
        }

        // Ignore bashes.
        if (a_attacker->AsActorState()->GetAttackState() == RE::ATTACK_STATE_ENUM::kBash) {
            return nullptr;
        }

        const bool left = a_left != 0;
        auto* obj = a_attacker->GetEquippedObject(left);

        if (!obj && left) {
            obj = a_attacker->GetEquippedObject(false);
        }

        return obj ? obj->As<RE::TESObjectWEAP>() : nullptr;
    }

    bool IsEnchanted(RE::TESObjectWEAP* a_weapon)
    {
        if (!a_weapon) {
            return false;
        }

        if (a_weapon->formEnchanting) {
            return true;
        }

        if (a_weapon->HasKeywordString("WeapMaterialSilver")) {
            return true;
        }

        return false;
    }

    bool ShouldRejectPresetMeleeHit(const std::vector<std::string>& a_collisionLayers, RE::Actor* a_aggressor, char a_left)
    {
        constexpr auto weaponLayer = std::to_underlying(RE::COL_LAYER::kWeapon);
        constexpr auto spellLayer = std::to_underlying(RE::COL_LAYER::kSpell);

        const auto& weaponName = kCollisionLayerNames[weaponLayer];
        const auto& spellName = kCollisionLayerNames[spellLayer];

        if (!CollisionLayers::IsLayerOverridden(a_collisionLayers, weaponName)) {
            return false;
        }

        // Weapon collision is disabled. Allow enchanted attacks if spell collision is enabled.
        // Enchanted weapons count as spells.
        if (!CollisionLayers::IsLayerOverridden(a_collisionLayers, spellName)) {
            auto* weapon = GetMeleeHitWeapon(a_aggressor, a_left);

            if (weapon && IsEnchanted(weapon)) {
                return false;
            }
        }

        return true;
    }

    bool ShouldRejectMeleeHit(RE::Actor* a_aggressor, RE::Actor* a_victim, int64_t a3, char a4)
    {
        constexpr auto weaponLayer = std::to_underlying(RE::COL_LAYER::kWeapon);
        constexpr auto spellLayer = std::to_underlying(RE::COL_LAYER::kSpell);

        if (!a_victim || weaponLayer >= CollisionLayers::kCollisionLayerNames.size() || spellLayer >= CollisionLayers::kCollisionLayerNames.size()) {
            return false;
        }

        auto& manager = VCD::Manager::GetSingleton();

        // Player attacking an NPC.
        if (a_aggressor && a_aggressor->IsPlayerRef() && !a_victim->IsPlayerRef()) {
            const auto& state = Dynamics::GetPresetState();
            const auto* playerConfig = manager.GetPresetConfig(state.current);

            if (playerConfig && ShouldRejectPresetMeleeHit(playerConfig->collisionLayers, a_aggressor, a4)) {
                return true;
            }
        }

        // Player being attacked.
        if (a_victim->IsPlayerRef()) {
            const auto& state = Dynamics::GetPresetState();
            const auto* presetConfig = manager.GetPresetConfig(state.current);

            if (presetConfig && ShouldRejectPresetMeleeHit(presetConfig->collisionLayers, a_aggressor, a4)) {
                logger::debug(
                    "[Melee Hit] Weapon collision disabled for player preset {}, ignoring hit from {} [{:08X}]",
                    VCD::PresetName(state.current),
                    a_aggressor ? a_aggressor->GetDisplayFullName() : "null",
                    a_aggressor ? a_aggressor->GetFormID() : 0);

                return true;
            }

            return false;
        }

        if (!Settings::GetSettings().enableNPCDynamics) {
            return false;
        }

        const auto* actorState = Dynamics::FindNPCPresetState(a_victim->GetFormID());

        VCD::Preset preset;

        if (actorState) {
            preset = actorState->preset;
        }
        else if (a_victim->HasKeywordString("ActorTypeGhost")) {
            preset = a_victim->IsInCombat() ? VCD::Preset::kGhostCombat : VCD::Preset::kGhostNeutral;
        }
        else {
            return false;
        }

        const auto* presetConfig = manager.GetPresetConfig(preset);

        if (!presetConfig) {
            return false;
        }

        const auto* actorLayers = Settings::GetNPCActorPresetCollisionLayers(a_victim->GetFormID(), preset);
        const auto* raceLayers = VCD::Race::GetRaceCollisionLayers(a_victim->GetRace());

        const auto& collisionLayers = actorLayers ? *actorLayers : raceLayers ? *raceLayers : presetConfig->collisionLayers;

        if (ShouldRejectPresetMeleeHit(collisionLayers, a_aggressor, a4)) {
            logger::debug(
                "[Melee Hit] Weapon collision disabled for {} [{:08X}] preset {}, ignoring hit",
                a_victim->GetDisplayFullName(),
                a_victim->GetFormID(),
                VCD::PresetName(preset));

            return true;
        }

        return false;
    }
}