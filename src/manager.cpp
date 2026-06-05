#include "manager.hpp"
#include "logger.hpp"
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "TrueHUDAPI.h"
#include "globals.hpp"

#include <type_traits>

using namespace VCD;

Manager& Manager::GetSingleton()
{
    static Manager singleton;
    return singleton;
}

Manager::Manager() :
    presetMeshes{ {
        PresetMesh{
            Preset::kVanillaLike,
            "Vanilla-like",
            MakeTemplatePath("VanillaLike.nif")
        },
        PresetMesh{
            Preset::kPersonalSpace,
            "Personal Space",
            MakeTemplatePath("PersonalSpace.nif")
        },
        PresetMesh{
            Preset::kCompact,
            "Compact",
            MakeTemplatePath("Compact.nif")
        },
        PresetMesh{
            Preset::kBulky,
            "Bulky",
            MakeTemplatePath("Bulky.nif")
        }
    } }
{ }

void Manager::LoadPresetMeshes()
{
    logger::info("Loading collision presets..");

    ClearLoadedMeshes();

    std::size_t loadedCount = 0;
    for (auto& presetMesh : presetMeshes) {
        if (LoadPresetMesh(presetMesh)) {
            ++loadedCount;
        }
    }

    logger::info("loading finished: {}/{} loaded successfully", loadedCount, presetMeshes.size());
}

void Manager::ClearLoadedMeshes()
{
    for (auto& presetMesh : presetMeshes) {
        presetMesh.root = nullptr;
        presetMesh.spCollisionObject = nullptr;
        presetMesh.data = {};
        presetMesh.loaded = false;
        presetMesh.foundCharacterBumper = false;
        presetMesh.foundBhkSPCollisionObject = false;
        presetMesh.foundCapsuleShape = false;
        presetMesh.loadResult = {};
    }
}

bool Manager::SetCollisionShape(RE::bhkCharProxyController* a_controller, const RE::hkpShape* a_shape)
{
    if (!a_controller || !a_shape)
        return false;

    auto* proxy = skyrim_cast<RE::hkpCharacterProxy*>(a_controller->proxy.referencedObject.get());

    if (!proxy || !proxy->shapePhantom)
        return false;

    proxy->shapePhantom->SetShape(a_shape);
    return true;
}

bool Manager::SetPreset(const RE::Actor* a_actor, const VCD::Preset& a_preset)
{
    if (!a_actor)
        return false;

    auto* playerController = skyrim_cast<RE::bhkCharProxyController*>(a_actor->GetCharController());

    if (!playerController)
        return false;

    const auto* mesh = GetPresetMesh(a_preset);
    if (!mesh) {
        return false;
    }

    const auto* capsuleShape = GetPresetShape(a_preset);
    if (!capsuleShape) {
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
        logger::error("Could not find world CharacterBumper capsule for preset [{}]. worldShapeType={}", mesh->name, worldShape ? static_cast<std::uint32_t>(worldShape->type) : 0);
        return false;
    }

    const auto previousRadius = worldCapsuleShape->radius;
    const auto previousHeight = worldCapsuleShape->vertexA.GetDistance3(worldCapsuleShape->vertexB);

    worldCapsuleShape->radius = capsuleShape->radius;
    worldCapsuleShape->vertexA = capsuleShape->vertexA;
    worldCapsuleShape->vertexB = capsuleShape->vertexB;

    logger::info("Applied preset [{}] to world CharacterBumper capsule. translation=({}, {}, {}), scale={}, radius {} -> {}, height {} -> {}",
        mesh->name,
        mesh->data.bump.translation.x,
        mesh->data.bump.translation.y,
        mesh->data.bump.translation.z,
        mesh->data.bump.scale,
        previousRadius,
        worldCapsuleShape->radius,
        previousHeight,
        worldCapsuleShape->vertexA.GetDistance3(worldCapsuleShape->vertexB));
    return true;
}


bool Manager::LoadPresetMesh(PresetMesh& a_mesh)
{
    logger::info("Loading preset [{}] from [{}]", a_mesh.name, a_mesh.path);

    RE::NiPointer<RE::NiNode> loadedRoot;
    RE::BSModelDB::DBTraits::ArgsType args{};

    const auto result = RE::BSModelDB::Demand(a_mesh.path.c_str(), loadedRoot, args);
    a_mesh.loadResult = result;

    if (result != RE::BSResource::ErrorCode::kNone || !loadedRoot) {
        logger::error("Failed to load preset [{}]. path=[{}], errorCode={}", a_mesh.name, a_mesh.path, static_cast<int>(result));
        return false;
    }

    // We should have the Havok object created by now.

    a_mesh.root = loadedRoot;
    a_mesh.loaded = true;

    auto* bumperObject = FindCharacterBumper(loadedRoot.get());
    if (!bumperObject) {
        logger::error("Preset [{}] loaded, but node [{}] was not found in [{}]", a_mesh.name, kCharacterBumperNodeName, a_mesh.path);
        return false;
    }

    a_mesh.foundCharacterBumper = true;

    a_mesh.data.bump.translation = bumperObject->local.translate;
    a_mesh.data.bump.scale = bumperObject->local.scale;

    auto* collisionObject = bumperObject->collisionObject.get();
    if (!collisionObject) {
        logger::error("Preset [{}] loaded and CharacterBumper found, but it has no collision object", a_mesh.name);
        return false;
    }


    auto* spCollisionObject = netimmerse_cast<RE::bhkSPCollisionObject*>(collisionObject);
    a_mesh.foundBhkSPCollisionObject = spCollisionObject != nullptr;

    if (!spCollisionObject) {
        logger::error("Preset [{}] CharacterBumper collision object is not bhkSPCollisionObject", a_mesh.name);
        return false;
    }

    a_mesh.spCollisionObject = RE::NiPointer<RE::bhkSPCollisionObject>(spCollisionObject);

    auto* body = spCollisionObject->body.get();
    if (!body) {
        logger::error("Preset [{}] bhkSPCollisionObject has no body", a_mesh.name);
        return false;
    }

    auto* bhkPhantom = skyrim_cast<RE::bhkShapePhantom*>(body);
    if (!bhkPhantom) {
        logger::error("Preset [{}] bhkSPCollisionObject body is not bhkShapePhantom", a_mesh.name);
        return false;
    }

    auto* referencedObject = bhkPhantom->referencedObject.get();
    if (!referencedObject) {
        logger::error("Preset [{}] bhkShapePhantom has no referenced Havok object", a_mesh.name);
        return false;
    }

    auto* simpleShapePhantom = skyrim_cast<RE::hkpSimpleShapePhantom*>(referencedObject);

    if (!simpleShapePhantom) {
        logger::error("Preset [{}] referenced object is not hkpSimpleShapePhantom", a_mesh.name);
        return false;
    }

    const auto* shape = simpleShapePhantom->collidable.shape;
    if (!shape) {
        logger::error("Preset [{}] hkpSimpleShapePhantom has no collidable shape", a_mesh.name);
        return false;
    }

    if (shape->type != RE::hkpShapeType::kCapsule) {
        logger::error("Preset [{}] shape is not capsule. shapeType={}", a_mesh.name, static_cast<std::uint32_t>(shape->type));
        return false;
    }

    const auto* capsuleShape = skyrim_cast<const RE::hkpCapsuleShape*>(shape);
    if (!capsuleShape) {
        logger::error("Preset [{}] failed to cast shape to hkpCapsuleShape", a_mesh.name);
        return false;
    }

    a_mesh.foundCapsuleShape = true;

    a_mesh.data.capsule.radius = capsuleShape->radius;
    a_mesh.data.capsule.point1.Set(capsuleShape->vertexA);
    a_mesh.data.capsule.point2.Set(capsuleShape->vertexB);
    a_mesh.data.capsule.height = capsuleShape->vertexA.GetDistance3(capsuleShape->vertexB);

    a_mesh.data.Log();

    return true;
}

RE::NiAVObject* Manager::FindCharacterBumper(RE::NiNode* a_root) const
{
    if (!a_root) {
        return nullptr;
    }

    const RE::BSFixedString bumperName(kCharacterBumperNodeName.data());

    if (a_root->name == bumperName) {
        return a_root;
    }

    return a_root->GetObjectByName(bumperName);
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

bool Manager::AreAllPresetMeshesLoaded() const
{
    for (const auto& presetMesh : presetMeshes) {
        if (!presetMesh.loaded ||
            !presetMesh.foundCharacterBumper ||
            !presetMesh.foundBhkSPCollisionObject ||
            !presetMesh.foundCapsuleShape) {
            return false;
        }
    }

    return true;
}

std::size_t Manager::GetLoadedPresetCount() const
{
    std::size_t count = 0;
    for (const auto& presetMesh : presetMeshes) {
        if (presetMesh.loaded &&
            presetMesh.foundCharacterBumper &&
            presetMesh.foundBhkSPCollisionObject &&
            presetMesh.foundCapsuleShape) {
            ++count;
        }
    }

    return count;
}

const std::array<PresetMesh, 4>& Manager::GetPresetMeshes() const noexcept
{
    return presetMeshes;
}

const PresetMesh* Manager::GetPresetMesh(const VCD::Preset& a_preset) const
{
    const auto index = static_cast<std::size_t>(ToUnderlying(a_preset));
    if (index >= presetMeshes.size()) {
        return nullptr;
    }

    return &presetMeshes[index];
}

RE::hkpCapsuleShape* Manager::GetPresetShape(const VCD::Preset& a_preset)
{
    return const_cast<RE::hkpCapsuleShape*>(static_cast<const Manager*>(this)->GetPresetShape(a_preset));
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

const RE::hkpCapsuleShape* Manager::GetPresetShape(const VCD::Preset& a_preset) const
{
    const auto* mesh = GetPresetMesh(a_preset);
    if (!mesh || !mesh->loaded || !mesh->foundCapsuleShape) {
        return nullptr;
    }

    const auto* spCollisionObject = mesh->spCollisionObject.get();
    if (!spCollisionObject || !spCollisionObject->body) {
        return nullptr;
    }

    const auto* bhkPhantom = static_cast<const RE::bhkShapePhantom*>(spCollisionObject->body.get());
    if (!bhkPhantom) {
        return nullptr;
    }

    const auto* referencedObject = bhkPhantom->referencedObject.get();
    if (!referencedObject) {
        return nullptr;
    }

    const auto* simpleShapePhantom = skyrim_cast<const RE::hkpSimpleShapePhantom*>(referencedObject);
    if (!simpleShapePhantom) {
        return nullptr;
    }

    const auto* shape = simpleShapePhantom->collidable.shape;
    if (!shape || shape->type != RE::hkpShapeType::kCapsule) {
        return nullptr;
    }

    return skyrim_cast<const RE::hkpCapsuleShape*>(shape);
}
