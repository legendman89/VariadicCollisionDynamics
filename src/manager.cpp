#include "manager.hpp"
#include "config.hpp"
#include "logger.hpp"
#include "helper.hpp"
#include "plugin.hpp"

#include <type_traits>

using namespace VCD;

Manager& Manager::GetSingleton()
{
	static Manager singleton;
	return singleton;
}

Manager::Manager() :
    presetConfigs{ {
        PresetConfig{
            Preset::kVanillaLike,
            PresetName(Preset::kVanillaLike)
        },
        PresetConfig{
            Preset::kPersonalSpace,
            PresetName(Preset::kPersonalSpace)
        },
        PresetConfig{
            Preset::kCompact,
            PresetName(Preset::kCompact)
        },
        PresetConfig{
            Preset::kBulky,
            PresetName(Preset::kBulky)
        },
        PresetConfig{
            Preset::kWerewolf,
            PresetName(Preset::kWerewolf)
        },
        PresetConfig{
            Preset::kVampireLord,
            PresetName(Preset::kVampireLord)
        }
    } }
{
    defaultPresetConfigs = presetConfigs;
}

void Manager::LoadPresets()
{
    logger::info("Loading collision presets from JSON..");

    ClearLoadedPresets();

    LoadPresetConfigurations(presetConfigs);

    defaultPresetConfigs = presetConfigs;
}

void Manager::ClearLoadedPresets()
{
    for (auto& presetConfig : presetConfigs) {
        presetConfig.data = {};
        presetConfig.loaded = false;
        presetConfig.configPath = {};
    }
}

bool Manager::SetPreset(const RE::Actor* a_actor, const VCD::Preset& a_preset, const bool& a_log)
{
    const auto* presetConfig = GetPresetConfig(a_preset);
    if (!presetConfig) {
        return false;
    }

    return SetCollisionData(a_actor, presetConfig->data, a_preset, presetConfig->name.c_str(), a_log);
}

bool Manager::SetCollisionData(const RE::Actor* a_actor, const VCD::CollisionData& a_data, const VCD::Preset& a_anchorPreset, const char* a_name, const bool& a_log)
{
    ActorBumperContext context{};
    if (!GetActorBumperContext(a_actor, context, a_log)) {
        return false;
    }

    RE::BSWriteLockGuard lock(context.world->worldLock);

    auto* worldCapsuleShape = FindWorldCharacterBumperShape(context.controller);
    if (!worldCapsuleShape) {
        if (a_log) {
            logger::error("Could not apply preset [{}]. CharacterBumper capsule unavailable", a_name ? a_name : "Unknown");
        }
        return false;
    }

    const auto previousRadius = worldCapsuleShape->radius;
    const auto previousVertexA = worldCapsuleShape->vertexA;
    const auto previousVertexB = worldCapsuleShape->vertexB;
    const auto previousHeight = worldCapsuleShape->vertexA.GetDistance3(worldCapsuleShape->vertexB);
    const auto previousCenter = (ToNiPoint3(previousVertexA) + ToNiPoint3(previousVertexB)) * 0.5F;
    const auto previousPosition = previousCenter * GetPresetScale();
    auto& bumperAnchorState = bumperAnchorStates[a_actor->GetFormID()];
    if (!bumperAnchorState.valid || bumperAnchorState.preset != a_anchorPreset) {
        bumperAnchorState.preset = a_anchorPreset;
        bumperAnchorState.centerZ = previousCenter.z;
        bumperAnchorState.valid = true;
    }

    const auto mappedRadius = a_data.capsule.radius;
    const auto mappedX = a_data.bump.translation.x * RE::bhkWorld::GetWorldScale();
    const auto mappedY = a_data.bump.translation.y * RE::bhkWorld::GetWorldScale();
    const auto presetPoint1Z = a_data.capsule.point1.z;
    const auto presetPoint2Z = a_data.capsule.point2.z;
    const auto* defaultPresetConfig = GetDefaultPresetConfig(a_anchorPreset);
    const auto defaultPoint1Z = defaultPresetConfig ? defaultPresetConfig->data.capsule.point1.z : presetPoint1Z;
    const auto defaultPoint2Z = defaultPresetConfig ? defaultPresetConfig->data.capsule.point2.z : presetPoint2Z;
    const auto defaultCenterZ = (defaultPoint1Z + defaultPoint2Z) * 0.5F;
    const auto anchorCenterZ = bumperAnchorState.centerZ;
    const auto mappedPoint1Z = anchorCenterZ + (presetPoint1Z - defaultCenterZ);
    const auto mappedPoint2Z = anchorCenterZ - (defaultCenterZ - presetPoint2Z);
    const auto mappedCenterZ = (mappedPoint1Z + mappedPoint2Z) * 0.5F;
    const auto mappedCenter = RE::NiPoint3(mappedX, mappedY, mappedCenterZ);
    const auto mappedPosition = mappedCenter * GetPresetScale();
    const auto mappedVertexA = ToHkVector(RE::NiPoint3(mappedCenter.x, mappedCenter.y, mappedPoint1Z));
    const auto mappedVertexB = ToHkVector(RE::NiPoint3(mappedCenter.x, mappedCenter.y, mappedPoint2Z));

    worldCapsuleShape->radius = mappedRadius;
    worldCapsuleShape->vertexA = mappedVertexA;
    worldCapsuleShape->vertexB = mappedVertexB;

    if (a_log) {
        logger::info(
            "\n"
            "  Center         : ({}, {}, {}) -> ({}, {}, {}), \n"
            "  Default Z      : top {}, bottom {}, center {}, \n"
            "  Anchor  Z      : {}, \n"
            "  Mapped  Z      : top {}, bottom {}, center {}, \n"
            "  Radius         : {} -> {}, \n"
            "  Height         : {} -> {}",
            previousPosition.x,
            previousPosition.y,
            previousPosition.z,
            mappedPosition.x,
            mappedPosition.y,
            mappedPosition.z,
            defaultPoint1Z,
            defaultPoint2Z,
            defaultCenterZ,
            anchorCenterZ,
            mappedPoint1Z,
			mappedPoint2Z,
            mappedCenterZ,
            previousRadius,
            worldCapsuleShape->radius,
            previousHeight,
            worldCapsuleShape->vertexA.GetDistance3(worldCapsuleShape->vertexB)
        );
    }
    return true;
}


RE::hkpCapsuleShape* Manager::FindWorldCharacterBumperShape(RE::bhkCharacterController* a_controller) const
{
    if (!a_controller) {
        LogCharacterBumperFailure("controller unavailable");
        return nullptr;
    }

    RE::hkpShape* shape = nullptr;
    if (auto* proxyController = skyrim_cast<RE::bhkCharProxyController*>(a_controller)) {
        RE::hkpCharacterProxy* proxy = proxyController->GetCharacterProxy();
        if (!proxy) {
            LogCharacterBumperFailure("hkpCharacterProxy unavailable");
            return nullptr;
        }

        if (!proxy->shapePhantom) {
            LogCharacterBumperFailure("shapePhantom unavailable");
            return nullptr;
        }

        shape = const_cast<RE::hkpShape*>(proxy->shapePhantom->collidable.shape);
        if (!shape) {
            LogCharacterBumperFailure("collision shape unavailable");
            return nullptr;
        }
    }
    else if (auto* rigidBodyController = skyrim_cast<RE::bhkCharRigidBodyController*>(a_controller)) {
        RE::hkpRigidBody* rigidBody = rigidBodyController->GetRigidBody();
        if (!rigidBody) {
            LogCharacterBumperFailure("hkpRigidBody unavailable");
            return nullptr;
        }

        if (!rigidBody->GetCollidable()) {
            LogCharacterBumperFailure("collidable unavailable");
            return nullptr;
        }

		shape = const_cast<RE::hkpShape*>(rigidBody->GetCollidable()->GetShape());
        if (!shape) {
            LogCharacterBumperFailure("collision shape unavailable");
            return nullptr;
        }
    }
    else {
        LogCharacterBumperFailure("unsupported character controller type");
        return nullptr;
    }

    if (auto* bumperShape = FindCharacterBumperShape(shape, RE::HK_INVALID_SHAPE_KEY)) {
        ClearCharacterBumperFailure();
        return bumperShape;
    }

    CapsuleCandidate candidate{};
    FindLargestCapsuleShape(shape, candidate);
    if (candidate.capsule) {
        ClearCharacterBumperFailure();
        logger::debug("Using fallback CharacterBumper capsule. radius={}, height={}", candidate.capsule->radius, candidate.height);
        return candidate.capsule;
    }

    LogCharacterBumperFailure("CharacterBumper capsule not found", static_cast<std::uint32_t>(shape->type));
    return nullptr;
}

bool Manager::GetActorBumperContext(const RE::Actor* a_actor, ActorBumperContext& a_context, const bool& a_log) const
{
    if (!a_actor) {
        if (a_log) {
            LogCharacterBumperFailure("actor unavailable");
        }
        return false;
    }

    auto* actorController = a_actor->GetCharController();
    if (!actorController) {
        if (a_log) {
            LogCharacterBumperFailure("character controller unavailable");
        }
        return false;
    }

    auto* cell = a_actor->GetParentCell();
    if (!cell) {
        if (a_log) {
            LogCharacterBumperFailure("parent cell unavailable");
        }
        return false;
    }

    auto* world = cell->GetbhkWorld();
    if (!world) {
        if (a_log) {
            LogCharacterBumperFailure("bhkWorld unavailable");
        }
        return false;
    }

    a_context.controller = actorController;
    a_context.world = world;
    return true;
}

RE::hkpCapsuleShape* Manager::FindCharacterBumperShape(RE::hkpShape* a_shape, const RE::hkpShapeKey& a_key) const
{
    if (!a_shape) {
        return nullptr;
    }

    if (a_shape->type == RE::hkpShapeType::kCapsule && IsCharacterBumperShape(a_shape, a_key)) {
        return skyrim_cast<RE::hkpCapsuleShape*>(a_shape);
    }

    // Inspired by the DCA mod.
    if (a_shape->type == RE::hkpShapeType::kList) {
        auto* listShape = skyrim_cast<RE::hkpListShape*>(a_shape);
        if (!listShape) {
            return nullptr;
        }

        for (auto& childInfo : listShape->childInfo) {
            auto* childShape = const_cast<RE::hkpShape*>(childInfo.shape);
            if (childShape && childShape->type == RE::hkpShapeType::kCapsule && IsCharacterBumperShape(childShape, RE::HK_INVALID_SHAPE_KEY)) {
                return skyrim_cast<RE::hkpCapsuleShape*>(childShape);
            }

            if (auto* bumperShape = FindCharacterBumperShape(childShape, RE::HK_INVALID_SHAPE_KEY)) {
                return bumperShape;
            }
        }

        return nullptr;
    }

    const auto* container = a_shape->GetContainer();
    if (!container) {
        return nullptr;
    }

    for (auto key = container->GetFirstKey(); key != RE::HK_INVALID_SHAPE_KEY; key = container->GetNextKey(key)) {
        RE::hkpShapeBuffer buffer{};
        auto* childShape = const_cast<RE::hkpShape*>(container->GetChildShape(key, buffer));
        if (childShape && childShape->type == RE::hkpShapeType::kCapsule && IsCharacterBumperShape(a_shape, key)) {
            return skyrim_cast<RE::hkpCapsuleShape*>(childShape);
        }

        if (auto* bumperShape = FindCharacterBumperShape(childShape, key)) {
            return bumperShape;
        }
    }

    return nullptr;
}

void Manager::FindLargestCapsuleShape(RE::hkpShape* a_shape, CapsuleCandidate& a_candidate) const
{
    if (!a_shape) {
        return;
    }

    if (a_shape->type == RE::hkpShapeType::kCapsule) {
        auto* capsule = skyrim_cast<RE::hkpCapsuleShape*>(a_shape);
        const auto height = capsule ? capsule->vertexA.GetDistance3(capsule->vertexB) : 0.0F;
        if (capsule && height > a_candidate.height) {
            a_candidate.capsule = capsule;
            a_candidate.height = height;
        }

        return;
    }

    if (a_shape->type == RE::hkpShapeType::kList) {
        auto* listShape = skyrim_cast<RE::hkpListShape*>(a_shape);
        if (!listShape) {
            return;
        }

        for (auto& childInfo : listShape->childInfo) {
            FindLargestCapsuleShape(const_cast<RE::hkpShape*>(childInfo.shape), a_candidate);
        }

        return;
    }

    const auto* container = a_shape->GetContainer();
    if (!container) {
        return;
    }

    for (auto key = container->GetFirstKey(); key != RE::HK_INVALID_SHAPE_KEY; key = container->GetNextKey(key)) {
        RE::hkpShapeBuffer buffer{};
        auto* childShape = const_cast<RE::hkpShape*>(container->GetChildShape(key, buffer));
        FindLargestCapsuleShape(childShape, a_candidate);
    }
}

bool Manager::IsCharacterBumperShape(const RE::hkpShape* a_shape, const RE::hkpShapeKey& a_key) const
{
    if (!a_shape || !a_shape->userData) {
        return false;
    }

    if (IsCharacterBumperMaterial(a_shape->userData->materialID)) {
        return true;
    }

    if (a_key != RE::HK_INVALID_SHAPE_KEY) {
        return IsCharacterBumperMaterial(a_shape->userData->GetMaterialID(a_key));
    }

    return false;
}

bool Manager::IsCharacterBumperMaterial(const RE::MATERIAL_ID& a_materialID) const
{
    if (a_materialID == RE::MATERIAL_ID::kCharacterBumper) {
        return true;
    }

    return a_materialID == RE::MATERIAL_ID::kTrap;
}

Manager::LookupFailureState& Manager::GetLookupFailureState()
{
    static LookupFailureState state{};
    return state;
}

void Manager::LogCharacterBumperFailure(const char* a_reason) const
{
    auto& state = GetLookupFailureState();
    const auto shapeType = std::numeric_limits<std::uint32_t>::max();
    if (state.reason == a_reason && state.shapeType == shapeType) {
        return;
    }

    logger::warn("CharacterBumper lookup failed: {}", a_reason);
    state.reason = a_reason;
    state.shapeType = shapeType;
}

void Manager::LogCharacterBumperFailure(const char* a_reason, const std::uint32_t& a_shapeType) const
{
    auto& state = GetLookupFailureState();
    if (state.reason == a_reason && state.shapeType == a_shapeType) {
        return;
    }

    logger::warn("CharacterBumper lookup failed: {}. rootShapeType={}", a_reason, a_shapeType);
    state.reason = a_reason;
    state.shapeType = a_shapeType;
}

void Manager::ClearCharacterBumperFailure() const
{
    auto& state = GetLookupFailureState();
    state.reason = nullptr;
    state.shapeType = std::numeric_limits<std::uint32_t>::max();
}

bool Manager::AreAllPresetsLoaded() const
{
    for (const auto& presetConfig : presetConfigs) {
        if (!presetConfig.loaded) {
            return false;
        }
    }

    return true;
}

size_t Manager::GetLoadedPresetCount() const
{
    size_t count = 0;
    for (const auto& presetConfig : presetConfigs) {
        if (presetConfig.loaded) {
            ++count;
        }
    }

    return count;
}

const std::array<PresetConfig, kPresetCount>& Manager::GetPresetConfigs() const noexcept
{
    return presetConfigs;
}

const PresetConfig* Manager::GetPresetConfig(const VCD::Preset& a_preset) const
{
    const auto index = static_cast<size_t>(ToUnderlying(a_preset));
    if (index >= presetConfigs.size()) {
        return nullptr;
    }

    return &presetConfigs[index];
}

PresetConfig* Manager::GetPresetConfig(const VCD::Preset& a_preset)
{
    return const_cast<PresetConfig*>(static_cast<const Manager*>(this)->GetPresetConfig(a_preset));
}

const PresetConfig* Manager::GetDefaultPresetConfig(const VCD::Preset& a_preset) const
{
    const auto index = static_cast<size_t>(ToUnderlying(a_preset));
    if (index >= defaultPresetConfigs.size()) {
        return nullptr;
    }

    return &defaultPresetConfigs[index];
}

bool Manager::RestorePresetDefault(const VCD::Preset& a_preset)
{
    auto* presetConfig = GetPresetConfig(a_preset);
    const auto* defaultPresetConfig = GetDefaultPresetConfig(a_preset);

    if (!presetConfig || !defaultPresetConfig || !defaultPresetConfig->loaded) {
        return false;
    }

    *presetConfig = *defaultPresetConfig;
    return true;
}


