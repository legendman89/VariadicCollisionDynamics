#include "manager.hpp"
#include "logger.hpp"

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
        presetMesh.loaded = false;
        presetMesh.foundCharacterBumper = false;
        presetMesh.foundBhkSPCollisionObject = false;
        presetMesh.loadResult = {};
    }
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


    auto* spCollisionObject = skyrim_cast<RE::bhkSPCollisionObject*>(collisionObject);
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

    auto* bhkPhantom = static_cast<RE::bhkShapePhantom*>(body);
    if (!bhkPhantom) {
        logger::error("Preset [{}] bhkSPCollisionObject body is not bhkShapePhantom", a_mesh.name);
        return false;
    }

    auto* referencedObject = bhkPhantom->referencedObject.get();
    if (!referencedObject) {
        logger::error("Preset [{}] bhkShapePhantom has no referenced Havok object", a_mesh.name);
        return false;
    }

    auto* simpleShapePhantom = static_cast<RE::hkpSimpleShapePhantom*>(referencedObject);

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

    const auto* capsuleShape = static_cast<const RE::hkpCapsuleShape*>(shape);
    if (!capsuleShape) {
        logger::error("Preset [{}] failed to cast shape to hkpCapsuleShape", a_mesh.name);
        return false;
    }

    a_mesh.foundCapsuleShape = true;

    a_mesh.data.capsule.radius = capsuleShape->radius;
    a_mesh.data.capsule.point1.Set(capsuleShape->vertexA);
    a_mesh.data.capsule.point2.Set(capsuleShape->vertexA);
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

bool Manager::AreAllPresetMeshesLoaded() const
{
    for (const auto& presetMesh : presetMeshes) {
        if (!presetMesh.loaded ||
            !presetMesh.foundCharacterBumper ||
            !presetMesh.foundBhkSPCollisionObject) {
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
            presetMesh.foundBhkSPCollisionObject) {
            ++count;
        }
    }

    return count;
}

const std::array<PresetMesh, 4>& Manager::GetPresetMeshes() const noexcept
{
    return presetMeshes;
}
