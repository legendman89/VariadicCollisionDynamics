#include "manager.hpp"
#include "config.hpp"
#include "logger.hpp"
#include "helper.hpp"
#include "plugin.hpp"
#include "drawLines.hpp"

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

bool Manager::SetPreset(const RE::Actor* a_actor, const VCD::Preset& a_preset)
{
    if (!a_actor)
        return false;

    auto* playerController = skyrim_cast<RE::bhkCharProxyController*>(a_actor->GetCharController());

    if (!playerController)
        return false;

    const auto* presetConfig = GetPresetConfig(a_preset);
    if (!presetConfig) {
        return false;
    }

    auto* cell = a_actor->GetParentCell();
    if (!cell) {
        return false;
    }

    auto* world = cell->GetbhkWorld();
    if (!world) {
        return false;
    }

    RE::BSWriteLockGuard lock(world->worldLock);

    auto* worldCapsuleShape = FindWorldCharacterBumperShape(playerController);
    if (!worldCapsuleShape) {
        auto* proxy = skyrim_cast<RE::hkpCharacterProxy*>(playerController->proxy.referencedObject.get());
        const auto* worldShape = proxy && proxy->shapePhantom ? proxy->shapePhantom->collidable.shape : nullptr;
        logger::error("Could not find world CharacterBumper capsule for preset [{}]. worldShapeType={}", 
                        presetConfig->name, worldShape ? static_cast<std::uint32_t>(worldShape->type) : 0);
        return false;
    }

    const auto previousRadius = worldCapsuleShape->radius;
    const auto previousVertexA = worldCapsuleShape->vertexA;
    const auto previousVertexB = worldCapsuleShape->vertexB;
    const auto previousHeight = worldCapsuleShape->vertexA.GetDistance3(worldCapsuleShape->vertexB);
    const auto previousCenter = (ToNiPoint3(previousVertexA) + ToNiPoint3(previousVertexB)) * 0.5F;
    const auto previousPosition = previousCenter * GetPresetScale();

    const auto mappedRadius = presetConfig->data.capsule.radius;
    const auto mappedHeight = presetConfig->data.capsule.height;
    const auto mappedHalfHeight = mappedHeight * 0.5F;
    const auto mappedX = presetConfig->data.bump.translation.x * RE::bhkWorld::GetWorldScale();
    const auto mappedY = presetConfig->data.bump.translation.y * RE::bhkWorld::GetWorldScale();
    const auto mappedCenter = RE::NiPoint3(mappedX, mappedY, previousCenter.z);
    const auto mappedPosition = mappedCenter * GetPresetScale();
    const auto mappedVertexA = ToHkVector(RE::NiPoint3(mappedCenter.x, mappedCenter.y, mappedCenter.z + mappedHalfHeight));
    const auto mappedVertexB = ToHkVector(RE::NiPoint3(mappedCenter.x, mappedCenter.y, mappedCenter.z - mappedHalfHeight));

    worldCapsuleShape->radius = mappedRadius;
    worldCapsuleShape->vertexA = mappedVertexA;
    worldCapsuleShape->vertexB = mappedVertexB;

    logger::info(" Position = ({}, {}, {}) -> ({}, {}, {}), radius {} -> {}, height {} -> {}",
        previousPosition.x,
        previousPosition.y,
        previousPosition.z,
        mappedPosition.x,
        mappedPosition.y,
        mappedPosition.z,
        previousRadius,
        worldCapsuleShape->radius,
        previousHeight,
        worldCapsuleShape->vertexA.GetDistance3(worldCapsuleShape->vertexB));
    return true;
}


RE::hkpCapsuleShape* Manager::FindWorldCharacterBumperShape(RE::bhkCharProxyController* a_controller) const
{
    if (!a_controller) {
        return nullptr;
    }

    auto* proxy = skyrim_cast<RE::hkpCharacterProxy*>(a_controller->proxy.referencedObject.get());
    if (!proxy || !proxy->shapePhantom) {
        return nullptr;
    }

    auto* shape = const_cast<RE::hkpShape*>(proxy->shapePhantom->collidable.shape);
    return FindCharacterBumperShape(shape, RE::HK_INVALID_SHAPE_KEY);
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

bool Manager::IsCharacterBumperShape(const RE::hkpShape* a_shape, const RE::hkpShapeKey& a_key) const
{
    if (!a_shape || !a_shape->userData) {
        return false;
    }

    if (a_shape->userData->materialID == RE::MATERIAL_ID::kCharacterBumper) {
        return true;
    }

    if (a_key != RE::HK_INVALID_SHAPE_KEY && a_shape->userData->GetMaterialID(a_key) == RE::MATERIAL_ID::kCharacterBumper) {
        return true;
    }

    return false;
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

std::size_t Manager::GetLoadedPresetCount() const
{
    std::size_t count = 0;
    for (const auto& presetConfig : presetConfigs) {
        if (presetConfig.loaded) {
            ++count;
        }
    }

    return count;
}

const std::array<PresetConfig, 4>& Manager::GetPresetConfigs() const noexcept
{
    return presetConfigs;
}

const PresetConfig* Manager::GetPresetConfig(const VCD::Preset& a_preset) const
{
    const auto index = static_cast<std::size_t>(ToUnderlying(a_preset));
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
    const auto index = static_cast<std::size_t>(ToUnderlying(a_preset));
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


