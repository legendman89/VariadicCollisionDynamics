#include "manager.hpp"
#include "config.hpp"
#include "logger.hpp"
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "TrueHUDAPI.h"
#include "globals.hpp"

#include <type_traits>

using namespace VCD;

namespace {
    RE::hkVector4 ToHkVector(const Vec3& a_vec)
    {
        return RE::hkVector4(a_vec.x, a_vec.y, a_vec.z, 0.0f);
    }
}

Manager& Manager::GetSingleton()
{
    static Manager singleton;
    return singleton;
}

Manager::Manager() :
    presetConfigs{ {
        PresetConfig{
            Preset::kVanillaLike,
            "Vanilla-like"
        },
        PresetConfig{
            Preset::kPersonalSpace,
            "Personal Space"
        },
        PresetConfig{
            Preset::kCompact,
            "Compact"
        },
        PresetConfig{
            Preset::kBulky,
            "Bulky"
        }
    } }
{ }

void Manager::LoadPresets()
{
    logger::info("Loading collision presets from JSON..");

    ClearLoadedPresets();

    LoadPresetConfigurations(presetConfigs);
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
    const auto previousHeight = worldCapsuleShape->vertexA.GetDistance3(worldCapsuleShape->vertexB);

    worldCapsuleShape->radius = presetConfig->data.capsule.radius;
    worldCapsuleShape->vertexA = ToHkVector(presetConfig->data.capsule.point1);
    worldCapsuleShape->vertexB = ToHkVector(presetConfig->data.capsule.point2);

    logger::info("Applied preset [{}] to world CharacterBumper capsule. translation=({}, {}, {}), scale={}, radius {} -> {}, height {} -> {}",
        presetConfig->name,
        presetConfig->data.bump.translation.x,
        presetConfig->data.bump.translation.y,
        presetConfig->data.bump.translation.z,
        presetConfig->data.bump.scale,
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

void Manager::DrawPlayerBumper()
{
    if (!globals::g_trueHUD)
        return;
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player)
        return;
    auto charController = skyrim_cast<RE::bhkCharProxyController*>(player->GetCharController());
    if (!charController)
        return;
    auto cell = player->GetParentCell();
    if (!cell)
        return;
    auto world = cell->GetbhkWorld();
    if (!world)
        return;
    RE::BSReadLockGuard lock(world->worldLock);
    auto* bumper = VCD::Manager::GetSingleton().FindWorldCharacterBumperShape(charController);
    if (!bumper)
        return;

    auto hkToNi = [](const RE::hkVector4& v) {
        return RE::NiPoint3(v.quad.m128_f32[0], v.quad.m128_f32[1], v.quad.m128_f32[2]);
        };

    //scale vertex and bumper  everything by hk scale for collisions
    constexpr float hkScale = 70.f; 

    RE::hkVector4 controllerPosHK;
    charController->GetPosition(controllerPosHK, false);
    RE::NiPoint3 controllerPos = hkToNi(controllerPosHK) * hkScale;
    RE::NiPoint3 aLocal = hkToNi(bumper->vertexA) * hkScale;
    RE::NiPoint3 bLocal = hkToNi(bumper->vertexB) * hkScale;

    
    float radius = bumper->radius * hkScale;
    float yaw = -player->data.angle.z;
    float c = std::cos(yaw);
    float s = std::sin(yaw);


    // its a cylinder or capsule shape so idk if we need to even rotate but mabye helpfull for future shapes if needed
    auto rotate = [&](const RE::NiPoint3& p) {
        return RE::NiPoint3(p.x * c - p.y * s, p.x * s + p.y * c, p.z);
        };

    RE::NiPoint3 a = rotate(aLocal) + controllerPos;
    RE::NiPoint3 b = rotate(bLocal) + controllerPos;

    //I can move this func to qtr so we dont need true hud 
    globals::g_trueHUD->DrawCapsule(a, b, radius, 0.f, 0xFF4087FF, 1.f);
}

