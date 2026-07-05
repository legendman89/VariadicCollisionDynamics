#include "manager.hpp"
#include "config.hpp"
#include "havok.hpp"
#include "logger.hpp"
#include "helper.hpp"
#include "plugin.hpp"
#include "posefixes.hpp"
#include "settings.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <type_traits>

using namespace VCD;

Manager::Manager()
{
    InitializePresetConfigs(presetConfigs);
    defaultPresetConfigs = presetConfigs;
    RebuildPresetIndex();
}

void Manager::LoadPresets()
{
    logger::info("Loading collision presets from JSON..");

    ClearLoadedPresets();

    LoadPresetConfigurations(presetConfigs);

    defaultPresetConfigs = presetConfigs;
    RebuildPresetIndex();
}

void Manager::ClearLoadedPresets()
{
    InitializePresetConfigs(presetConfigs);
}

void Manager::ClearRuntimeState()
{
    bumperAnchorStates.clear();
    convexShapeStates.clear();
    actorStates.clear();
    actorVanillaCollisionData.clear();
}

void Manager::ClearActorRuntimeState(const RE::FormID& a_formID)
{
    bumperAnchorStates.erase(a_formID);
    convexShapeStates.erase(a_formID);
    actorStates.erase(a_formID);
    actorVanillaCollisionData.erase(a_formID);
}

void Manager::ClearActorTransientState(const RE::FormID& a_formID)
{
    bumperAnchorStates.erase(a_formID);
    actorStates.erase(a_formID);
}

bool Manager::SetPreset(const RE::Actor* a_actor, const Preset& a_preset, const PoseFlags& a_poseFlags, const bool& a_log, const bool& a_rebuildConvex)
{
    const auto* presetConfig = GetPresetConfig(a_preset);
    if (!presetConfig) {
        return false;
    }

    return SetCollisionData(a_actor, presetConfig->data, a_preset, presetConfig->name.c_str(), a_poseFlags, a_log, a_rebuildConvex);
}

const CollisionData* Manager::GetActorVanillaCollisionData(const RE::FormID& a_formID) const
{
    const auto it = actorVanillaCollisionData.find(a_formID);
    return it != actorVanillaCollisionData.end() ? &it->second : nullptr;
}

bool Manager::FixSittingPose(const RE::Actor* a_actor, const PoseFlags& a_poseFlags, const bool& a_log)
{
    ActorBumperContext context{};
    if (!GetActorBumperContext(a_actor, context, a_log)) {
        return false;
    }

    RE::BSWriteLockGuard lock(context.world->worldLock);

    CharacterBumperShape bumperShape{};
    if (!FindWorldCharacterBumperShapeData(context.controller, bumperShape)) {
        return false;
    }

    auto* worldCapsuleShape = bumperShape.capsule;
    if (!worldCapsuleShape) {
        return false;
    }

    const auto actorFormID = a_actor->GetFormID();
    auto& lastActorState = actorStates[actorFormID];

    if (!a_poseFlags.isSitting) {
        if (!lastActorState.sittingPoseApplied || !lastActorState.hasStandingCapsule) {
            return true;
        }

        ApplyCapsulePoseHeight(worldCapsuleShape, lastActorState.standingRadius, lastActorState.standingPoint1Z, lastActorState.standingPoint2Z);
        lastActorState.sittingPoseApplied = false;
        return true;
    }

    if (!lastActorState.sittingPoseApplied) {
        CacheStandingCapsule(lastActorState, ToNiPoint3(worldCapsuleShape->vertexA).z, ToNiPoint3(worldCapsuleShape->vertexB).z, worldCapsuleShape->radius);
    }

    auto mappedPoint1Z = lastActorState.standingPoint1Z;
    auto mappedPoint2Z = lastActorState.standingPoint2Z;
    const auto* player = RE::PlayerCharacter::GetSingleton();
    const auto scale = a_actor == player ? Settings::GetSettings().playerSittingScale : Settings::GetSettings().npcSittingScale;
    ApplySittingCapsule(lastActorState, mappedPoint1Z, mappedPoint2Z, lastActorState.standingRadius, a_poseFlags, scale);
    ApplyCapsulePoseHeight(worldCapsuleShape, lastActorState.standingRadius, mappedPoint1Z, mappedPoint2Z);
    lastActorState.sittingPoseApplied = true;
    return true;
}

bool Manager::FixSneakingPose(const RE::Actor* a_actor, const PoseFlags& a_poseFlags, const bool& a_log, const bool& a_rebuildConvex)
{
    ActorBumperContext context{};
    if (!GetActorBumperContext(a_actor, context, a_log)) {
        return false;
    }

    RE::BSWriteLockGuard lock(context.world->worldLock);

    CharacterBumperShape bumperShape{};
    if (!FindWorldCharacterBumperShapeData(context.controller, bumperShape)) {
        return false;
    }

    auto* worldCapsuleShape = bumperShape.capsule;
    if (!worldCapsuleShape) {
        return false;
    }

    auto& lastActorState = actorStates[a_actor->GetFormID()];
    if (!a_poseFlags.isSneaking) {
        if (!lastActorState.sneakingPoseApplied || !lastActorState.hasStandingCapsule) {
            return true;
        }

        ApplyCapsulePoseHeight(worldCapsuleShape, lastActorState.standingRadius, lastActorState.standingPoint1Z, lastActorState.standingPoint2Z);
        if (a_rebuildConvex && a_actor == RE::PlayerCharacter::GetSingleton() && lastActorState.hasStandingTranslation) {
            SetConvexShape(a_actor, context.controller, lastActorState.standingRadius, lastActorState.standingPoint1Z, lastActorState.standingPoint2Z, lastActorState.standingTranslation, "sneak_restore", a_log);
        }
        lastActorState.sneakingPoseApplied = false;
        return true;
    }

    if (a_poseFlags.isSitting) {
        return false;
    }

    if (!lastActorState.hasStandingCapsule) {
        CacheStandingCapsule(lastActorState, ToNiPoint3(worldCapsuleShape->vertexA).z, ToNiPoint3(worldCapsuleShape->vertexB).z, worldCapsuleShape->radius);
    }

    auto mappedPoint1Z = lastActorState.standingPoint1Z;
    auto mappedPoint2Z = lastActorState.standingPoint2Z;
    ApplySneakingCapsule(lastActorState, mappedPoint1Z, mappedPoint2Z, Settings::GetSettings().playerSneakingScale);
    ApplyCapsulePoseHeight(worldCapsuleShape, lastActorState.standingRadius, mappedPoint1Z, mappedPoint2Z);
    if (a_rebuildConvex && a_actor == RE::PlayerCharacter::GetSingleton() && lastActorState.hasStandingTranslation) {
        SetConvexShape(a_actor, context.controller, lastActorState.standingRadius, mappedPoint1Z, mappedPoint2Z, lastActorState.standingTranslation, "sneak_crouch", a_log);
    }
    lastActorState.sneakingPoseApplied = true;
    return true;
}

float Manager::GetStandingCapsuleHeight(const RE::Actor* a_actor) const
{
    if (!a_actor) {
        return 0.0F;
    }

    const auto it = actorStates.find(a_actor->GetFormID());
    if (it == actorStates.end() || !it->second.hasStandingCapsule) {
        return 0.0F;
    }

    return GetCapsuleHeight(it->second.standingPoint1Z, it->second.standingPoint2Z, it->second.standingRadius) * GetPresetScale();
}


RE::hkpCapsuleShape* Manager::FindWorldCharacterBumperShape(RE::bhkCharacterController* a_controller) const
{
    CharacterBumperShape bumperShape{};
    if (!FindWorldCharacterBumperShapeData(a_controller, bumperShape)) {
        return nullptr;
    }

    return bumperShape.capsule;
}

RE::hkpShape* Manager::GetControllerRootShape(RE::bhkCharacterController* a_controller) const
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

    return shape;
}

RE::hkpSimpleShapePhantom* Manager::GetCameraSimpleShapePhantom(RE::bhkSimpleShapePhantom* bhkPhantom)
{
    if (!bhkPhantom) {
        return nullptr;
    }

    auto* refObject = reinterpret_cast<RE::bhkRefObject*>(bhkPhantom);
    if (!refObject) {
        return nullptr;
    }

    return static_cast<RE::hkpSimpleShapePhantom*>(refObject->referencedObject.get());
}

RE::hkpSphereShape* Manager::GetCameraSphereShape(RE::bhkSimpleShapePhantom* bhkPhantom)
{
    auto* phantom = GetCameraSimpleShapePhantom(bhkPhantom);
    if (!phantom) {
        return nullptr;
    }

    auto* shape = const_cast<RE::hkpShape*>(phantom->collidable.shape);
    if (!shape || shape->type != RE::hkpShapeType::kSphere) {
        return nullptr;
    }

    return static_cast<RE::hkpSphereShape*>(shape);
}

bool Manager::FindWorldCharacterBumperShapeData(RE::bhkCharacterController* a_controller, CharacterBumperShape& a_bumper) const
{
    auto* shape = GetControllerRootShape(a_controller);
    if (!shape) {
        return false;
    }

    a_bumper.rootShape = shape;
    if (FindCharacterBumperShape(shape, RE::HK_INVALID_SHAPE_KEY, a_bumper)) {
        ClearCharacterBumperFailure();
        return true;
    }

    CapsuleCandidate candidate{};
    FindLargestCapsuleShape(shape, candidate);
    if (candidate.capsule) {
        ClearCharacterBumperFailure();
        logger::debug("Using fallback Bumper capsule. radius={}, height={}", candidate.capsule->radius, candidate.height);
        a_bumper.capsule = candidate.capsule;
        a_bumper.childInfo = candidate.childInfo;
        return true;
    }

    LogCharacterBumperFailure("Bumper capsule not found", static_cast<std::uint32_t>(shape->type));
    return false;
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

bool Manager::FindCharacterBumperShape(RE::hkpShape* a_shape, const RE::hkpShapeKey& a_key, CharacterBumperShape& a_bumper) const
{
    if (!a_shape) {
        return false;
    }

    if (a_shape->type == RE::hkpShapeType::kCapsule && IsCharacterBumperShape(a_shape, a_key)) {
        a_bumper.capsule = skyrim_cast<RE::hkpCapsuleShape*>(a_shape);
        return a_bumper.capsule;
    }

    if (a_shape->type == RE::hkpShapeType::kList) {
        auto* listShape = skyrim_cast<RE::hkpListShape*>(a_shape);
        if (!listShape) {
            return false;
        }

        for (auto& childInfo : listShape->childInfo) {
            auto* childShape = const_cast<RE::hkpShape*>(childInfo.shape);
            if (childShape && childShape->type == RE::hkpShapeType::kCapsule && IsCharacterBumperShape(childShape, RE::HK_INVALID_SHAPE_KEY)) {
                a_bumper.capsule = skyrim_cast<RE::hkpCapsuleShape*>(childShape);
                a_bumper.childInfo = &childInfo;
                return a_bumper.capsule;
            }

            auto nestedBumper = a_bumper;
            if (FindCharacterBumperShape(childShape, RE::HK_INVALID_SHAPE_KEY, nestedBumper)) {
                a_bumper = nestedBumper;
                return true;
            }
        }

        return false;
    }

    const auto* container = a_shape->GetContainer();
    if (!container) {
        return false;
    }

    for (auto key = container->GetFirstKey(); key != RE::HK_INVALID_SHAPE_KEY; key = container->GetNextKey(key)) {
        RE::hkpShapeBuffer buffer{};
        auto* childShape = const_cast<RE::hkpShape*>(container->GetChildShape(key, buffer));
        if (childShape && childShape->type == RE::hkpShapeType::kCapsule && IsCharacterBumperShape(a_shape, key)) {
            a_bumper.capsule = skyrim_cast<RE::hkpCapsuleShape*>(childShape);
            return a_bumper.capsule;
        }

        if (FindCharacterBumperShape(childShape, key, a_bumper)) {
            return true;
        }
    }

    return false;
}

void Manager::FindLargestCapsuleShape(RE::hkpShape* a_shape, CapsuleCandidate& a_candidate, RE::hkpListShape::ChildInfo* a_childInfo) const
{
    if (!a_shape) {
        return;
    }

    if (a_shape->type == RE::hkpShapeType::kCapsule) {
        auto* capsule = skyrim_cast<RE::hkpCapsuleShape*>(a_shape);
        const auto height = capsule ? capsule->vertexA.GetDistance3(capsule->vertexB) : 0.0F;
        if (capsule && height > a_candidate.height) {
            a_candidate.capsule = capsule;
            a_candidate.childInfo = a_childInfo;
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
            FindLargestCapsuleShape(const_cast<RE::hkpShape*>(childInfo.shape), a_candidate, &childInfo);
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
        FindLargestCapsuleShape(childShape, a_candidate, nullptr);
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

bool Manager::FindControllerConvexShape(RE::bhkCharacterController* a_controller, ConvexShapeData& a_convex) const
{
    auto* rootShape = GetControllerRootShape(a_controller);
    if (!rootShape) {
        return false;
    }

    a_convex.rootShape = rootShape;
    if (rootShape->type == RE::hkpShapeType::kList) {
        auto* listShape = skyrim_cast<RE::hkpListShape*>(rootShape);
        if (!listShape || listShape->childInfo.empty()) {
            return false;
        }

        auto& childInfo = listShape->childInfo[0];
        auto* childShape = const_cast<RE::hkpShape*>(childInfo.shape);
        if (!childShape || childShape->type != RE::hkpShapeType::kConvexVertices) {
            return false;
        }

        a_convex.listShape = listShape;
        a_convex.childInfo = &childInfo;
        a_convex.convexShape = skyrim_cast<RE::hkpConvexVerticesShape*>(childShape);
        return a_convex.convexShape;
    }

    if (rootShape->type != RE::hkpShapeType::kConvexVertices) {
        return false;
    }

    a_convex.convexShape = skyrim_cast<RE::hkpConvexVerticesShape*>(rootShape);
    return a_convex.convexShape;
}

bool Manager::CacheConvexShapeState(const RE::FormID& a_formID, const RE::bhkCharacterController* a_controller, const RE::hkpConvexVerticesShape* a_shape, ConvexShapeState& a_state)
{
    if (a_state.valid) {
        return true;
    }

    if (!a_shape) {
        return false;
    }

    RE::hkArray<RE::hkVector4> originalVertices{};
    Havok::GetOriginalVertices(a_shape, originalVertices);
    if (originalVertices.empty()) {
        logger::warn("Actor convex cache failed [{:08X}]: original vertices unavailable", a_formID);
        return false;
    }

    a_state.originalVertices.assign(originalVertices.begin(), originalVertices.end());
    a_state.controller = a_controller;
    a_state.currentShape = a_shape;
    const auto vertex = ToNiPoint3(a_state.originalVertices[0]);
    a_state.originalRadius = std::sqrt((vertex.x * vertex.x) + (vertex.y * vertex.y));
    a_state.valid = a_state.originalRadius > 0.0F;
    if (!a_state.valid) {
        logger::warn("Actor convex cache failed [{:08X}]: original radius unavailable", a_formID);
        return false;
    }

    logger::debug("Actor convex cache [{:08X}]: vertices={}, radius={}", a_formID, a_state.originalVertices.size(), a_state.originalRadius);
    return true;
}

bool Manager::ReplaceControllerConvexShape(RE::bhkCharacterController* a_controller, ConvexShapeData& a_convex, RE::hkpConvexVerticesShape* a_newShape) const
{
    if (!a_controller || !a_convex.convexShape || !a_newShape) {
        return false;
    }

    auto* oldShape = a_convex.convexShape;
    if (oldShape->userData) {
        oldShape->userData->SetReferencedObject(a_newShape);
    }

    if (a_convex.childInfo) {
        a_convex.childInfo->shape = a_newShape;
        oldShape->RemoveReference();
        a_convex.convexShape = a_newShape;
        return true;
    }

    bool replaced = false;
    if (auto* proxyController = skyrim_cast<RE::bhkCharProxyController*>(a_controller)) {
        auto* proxy = proxyController->GetCharacterProxy();
        if (proxy && proxy->shapePhantom) {
            proxy->shapePhantom->SetShape(a_newShape);
            replaced = true;
        }
    }
    else if (auto* rigidBodyController = skyrim_cast<RE::bhkCharRigidBodyController*>(a_controller)) {
        auto* rigidBody = rigidBodyController->GetRigidBody();
        if (rigidBody) {
            rigidBody->SetShape(a_newShape);
            replaced = true;
        }
    }

    if (replaced) {
        a_newShape->RemoveReference();
        a_convex.rootShape = a_newShape;
        a_convex.convexShape = a_newShape;
    }

    return replaced;
}

void Manager::LogCharacterBumperFailure(const char* a_reason) const
{
    auto& state = GetLookupFailureState();
    const auto shapeType = std::numeric_limits<std::uint32_t>::max();
    if (state.reason == a_reason && state.shapeType == shapeType) {
        return;
    }

    logger::warn("Bumper lookup failed: {}", a_reason);
    state.reason = a_reason;
    state.shapeType = shapeType;
}

void Manager::LogCharacterBumperFailure(const char* a_reason, const std::uint32_t& a_shapeType) const
{
    auto& state = GetLookupFailureState();
    if (state.reason == a_reason && state.shapeType == a_shapeType) {
        return;
    }

    logger::warn("Bumper lookup failed: {}. rootShapeType={}", a_reason, a_shapeType);
    state.reason = a_reason;
    state.shapeType = a_shapeType;
}

void Manager::ClearCharacterBumperFailure() const
{
    auto& state = GetLookupFailureState();
    state.reason = nullptr;
    state.shapeType = std::numeric_limits<std::uint32_t>::max();
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

bool Manager::RestorePresetDefault(const Preset& a_preset)
{
    auto* presetConfig = GetPresetConfig(a_preset);
    const auto* defaultPresetConfig = GetDefaultPresetConfig(a_preset);

    if (!presetConfig || !defaultPresetConfig || !defaultPresetConfig->loaded) {
        return false;
    }

    *presetConfig = *defaultPresetConfig;
    return true;
}

bool Manager::CreatePreset(const std::string& a_name, const CollisionData& a_data, Preset& a_preset, std::string& a_error)
{
    const auto first = std::find_if_not(a_name.begin(), a_name.end(),
        [](const unsigned char& a_character) {
            return std::isspace(a_character);
        }
    );
    const auto last = std::find_if_not(a_name.rbegin(), a_name.rend(),
        [](const unsigned char& a_character) {
            return std::isspace(a_character);
        }
    ).base();

    const std::string name = first < last ? std::string(first, last) : std::string{};
    const auto key = MakePresetKey(name);
    if (key.empty()) {
        a_error = "Enter a name containing letters or numbers.";
        return false;
    }

    if (GetPresetConfig(key)) {
        a_error = "A preset with this name already exists.";
        return false;
    }

    std::error_code ec;
    if (fs::exists(GetPresetDir() / (key + ".json"), ec)) {
        a_error = "A preset file with this name already exists.";
        return false;
    }

    PresetConfig config{};
    config.preset = static_cast<Preset>(presetConfigs.size());
    config.key = key;
    config.name = name;
    config.data = a_data;
    config.data.RecalculateHeight();
    config.loaded = true;
    config.builtIn = false;
    config.fileBacked = true;

    if (!SavePresetConfiguration(config, a_error)) {
        return false;
    }

    presetConfigs.push_back(config);
    defaultPresetConfigs.push_back(config);
    presetIndices.emplace(config.key, presetConfigs.size() - 1);
    presetNameIndices.emplace(config.name, presetConfigs.size() - 1);
    a_preset = config.preset;
    return true;
}

bool Manager::DeletePreset(const Preset& a_preset, std::string& a_key, std::string& a_error)
{
    const auto index = static_cast<size_t>(ToUnderlying(a_preset));
    if (index >= presetConfigs.size() || index >= defaultPresetConfigs.size()) {
        a_error = "The selected preset is unavailable.";
        return false;
    }

    const auto& config = presetConfigs[index];
    if (config.builtIn) {
        a_error = "Built-in presets cannot be deleted.";
        return false;
    }

    const auto path = GetPresetDir() / (config.key + ".json");
    std::error_code ec;
    fs::remove(path, ec);
    if (ec) {
        a_error = "Could not delete the preset file.";
        logger::error("Failed to delete preset {}: {}", ToUTF8(path), ec.message());
        return false;
    }

    a_key = config.key;
    presetConfigs.erase(presetConfigs.begin() + index);
    defaultPresetConfigs.erase(defaultPresetConfigs.begin() + index);
    for (size_t i = index; i < presetConfigs.size(); ++i) {
        presetConfigs[i].preset = static_cast<Preset>(i);
        defaultPresetConfigs[i].preset = static_cast<Preset>(i);
    }
    RebuildPresetIndex();
    logger::info("Preset deleted: {}", ToUTF8(path));
    return true;
}

void Manager::RebuildPresetIndex()
{
    presetIndices.clear();
    presetNameIndices.clear();
    presetIndices.reserve(presetConfigs.size());
    presetNameIndices.reserve(presetConfigs.size());
    for (size_t i = 0; i < presetConfigs.size(); ++i) {
        presetIndices[presetConfigs[i].key] = i;
        presetNameIndices.emplace(presetConfigs[i].name, i);
    }
}

bool Manager::IsCameraPreset(VCD::Preset a_preset) const
{
    static constexpr std::array cameraPresets = {
#define CAMERA_PRESET_COLLECT(S, D) D,
        FOREACH_CAMERA_PRESET_STATE(CAMERA_PRESET_COLLECT)
#undef CAMERA_PRESET_COLLECT
    };
    for (const auto& p : cameraPresets) {
        if (p == a_preset) return true;
    }
    return false;
}

