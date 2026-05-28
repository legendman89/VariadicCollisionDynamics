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
            R"(meshes\Variadic Collision Tweaks\Templates\VanillaLike.nif)"
        },
        PresetMesh{
            Preset::kPersonalSpace,
            "Personal Space",
            R"(meshes\Variadic Collision Tweaks\Templates\PersonalSpace.nif)"
        },
        PresetMesh{
            Preset::kCompact,
            "Compact",
            R"(meshes\Variadic Collision Tweaks\Templates\Compact.nif)"
        },
        PresetMesh{
            Preset::kBulky,
            "Bulky",
            R"(meshes\Variadic Collision Tweaks\Templates\Bulky.nif)"
        }
    } }
{
}

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

    const auto result = RE::BSModelDB::Demand(a_mesh.path.data(), loadedRoot, args);
    a_mesh.loadResult = result;

    if (result != RE::BSResource::ErrorCode::kNone || !loadedRoot) {
        logger::error("Failed to load preset [{}]. path=[{}], errorCode={}", a_mesh.name, a_mesh.path, static_cast<int>(result));
        return false;
    }

    a_mesh.root = loadedRoot;
    a_mesh.loaded = true;

    auto* bumperObject = FindCharacterBumper(loadedRoot.get());
    if (!bumperObject) {
        logger::error("Preset [{}] loaded, but node [{}] was not found in [{}]", a_mesh.name, kCharacterBumperNodeName, a_mesh.path);
        return false;
    }

    a_mesh.foundCharacterBumper = true;

    // Probably we won't need collisionObject but it's nice to check for safety before copying the character bumper node.
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

    logger::info("Preset [{}] OK", a_mesh.name, loadedRoot->name.c_str());

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
