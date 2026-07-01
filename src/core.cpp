#include "manager.hpp"
#include "havok.hpp"
#include "settings.hpp"
#include "dynamics.hpp"

using namespace VCD;

//Legendman please organize this how you want
// these globals are set in the menu editor for camera,
// then are used in thirdperson_SetRotation hook to apply camera collision pos


// Core logic of capsule shaping based on actor's preset and pose.
bool Manager::SetCollisionData(const RE::Actor* a_actor, const CollisionData& a_data, const Preset& a_anchorPreset, const char* a_name, const PoseFlags& a_poseFlags, const bool& a_log, const bool& a_rebuildConvex)
{
    ActorBumperContext context{};
    if (!GetActorBumperContext(a_actor, context, a_log)) {
        return false;
    }

    RE::BSWriteLockGuard lock(context.world->worldLock);

    CharacterBumperShape bumperShape{};
    if (!FindWorldCharacterBumperShapeData(context.controller, bumperShape)) {
        if (a_log) {
            logger::error("Could not apply preset [{}]. Bumper capsule unavailable", a_name ? a_name : "Unknown");
        }
        return false;
    }

    auto* worldCapsuleShape = bumperShape.capsule;
    if (!worldCapsuleShape) {
        if (a_log) {
            logger::error("Could not apply preset [{}]. Bumper capsule unavailable", a_name ? a_name : "Unknown");
        }
        return false;
    }

    const auto actorFormID = a_actor->GetFormID();
    const auto previousCenter = (ToNiPoint3(worldCapsuleShape->vertexA) + ToNiPoint3(worldCapsuleShape->vertexB)) * 0.5F;
    const auto previousPosition = previousCenter * GetPresetScale();
    auto& lastActorState = actorStates[actorFormID];
    auto& bumperAnchorState = bumperAnchorStates[actorFormID];
    const bool refreshSittingAnchor = !a_poseFlags.isSitting && bumperAnchorState.fromSitting;
    const bool useStandingAnchor = (a_poseFlags.isSitting || a_poseFlags.isSneaking) && lastActorState.hasStandingCapsule;
    if (!bumperAnchorState.valid || bumperAnchorState.preset != a_anchorPreset || refreshSittingAnchor) {
        if (refreshSittingAnchor) {
            const auto* name = const_cast<RE::Actor*>(a_actor)->GetDisplayFullName();
            logger::debug("Actor: {} [{:08X}] refreshed standing anchor after initial sitting anchor", name ? name : "Actor", actorFormID);
        }
        bumperAnchorState.preset = a_anchorPreset;
        bumperAnchorState.centerZ = useStandingAnchor ? (lastActorState.standingPoint1Z + lastActorState.standingPoint2Z) * 0.5F : previousCenter.z;
        bumperAnchorState.valid = true;
        bumperAnchorState.fromSitting = a_poseFlags.isSitting && !lastActorState.hasStandingCapsule;
    }

    const auto mappedRadius = a_data.capsule.radius;
    const auto presetPoint1Z = a_data.capsule.point1.z;
    const auto presetPoint2Z = a_data.capsule.point2.z;

    // Adjust according to actor state.
    const auto* defaultPresetConfig = GetDefaultPresetConfig(a_anchorPreset);
    const auto defaultPoint1Z = defaultPresetConfig ? defaultPresetConfig->data.capsule.point1.z : presetPoint1Z;
    const auto defaultPoint2Z = defaultPresetConfig ? defaultPresetConfig->data.capsule.point2.z : presetPoint2Z;
    const auto defaultCenterZ = (defaultPoint1Z + defaultPoint2Z) * 0.5F;
    const auto anchorCenterZ = bumperAnchorState.centerZ;
    auto mappedPoint1Z = anchorCenterZ + (presetPoint1Z - defaultCenterZ);
    auto mappedPoint2Z = anchorCenterZ - (defaultCenterZ - presetPoint2Z);
    const auto standingHeight = GetCapsuleHeight(mappedPoint1Z, mappedPoint2Z, mappedRadius);
    auto mappedHeight = standingHeight;
    const bool hadStandingCapsule = lastActorState.hasStandingCapsule;
    if (!a_poseFlags.isSitting || hadStandingCapsule) {
        CacheStandingCapsule(lastActorState, mappedPoint1Z, mappedPoint2Z, mappedRadius);
    }
    if (!a_poseFlags.isSitting) {
        lastActorState.standingTranslation = a_data.bump.translation;
        lastActorState.hasStandingTranslation = true;
    }
    if (a_poseFlags.isSitting) {
        if (!hadStandingCapsule) {
            const auto* name = const_cast<RE::Actor*>(a_actor)->GetDisplayFullName();
            logger::debug("Actor: {} [{:08X}] is sitting before standing capsule cache exists; using mapped preset", name ? name : "Actor", actorFormID);
            CacheStandingCapsule(lastActorState, mappedPoint1Z, mappedPoint2Z, mappedRadius);
        }

        const auto* player = RE::PlayerCharacter::GetSingleton();
        const auto scale = a_actor == player ? Settings::GetSettings().playerSittingScale : Settings::GetSettings().npcSittingScale;
        mappedHeight = ApplySittingCapsule(lastActorState, mappedPoint1Z, mappedPoint2Z, mappedRadius, a_poseFlags, scale);
    }
    const auto* player = RE::PlayerCharacter::GetSingleton();
    const auto isPlayerSneaking = player && a_actor == player && a_poseFlags.isSneaking && !a_poseFlags.isSitting;
    const auto isNPCSneaking = player && a_actor != player && a_poseFlags.isSneaking && !a_poseFlags.isSitting;
    if (isPlayerSneaking || isNPCSneaking) {
        const auto scale = isPlayerSneaking ? Settings::GetSettings().playerSneakingScale : Settings::GetSettings().npcSneakingScale;
        ApplySneakingCapsule(lastActorState, mappedPoint1Z, mappedPoint2Z, scale);
        mappedHeight = GetCapsuleHeight(mappedPoint1Z, mappedPoint2Z, mappedRadius);
    }

    // Compute mapped endpoints and their center.
    const auto mappedCenterZ = (mappedPoint1Z + mappedPoint2Z) * 0.5F; 
    const auto mappedX = a_data.bump.translation.x * RE::bhkWorld::GetWorldScale();
    const auto mappedY = a_data.bump.translation.y * RE::bhkWorld::GetWorldScale();
    const auto mappedCenter = RE::NiPoint3(mappedX, mappedY, mappedCenterZ);
    const auto mappedPosition = mappedCenter * GetPresetScale();
    const auto mappedVertexA = ToHkVector(RE::NiPoint3(mappedCenter.x, mappedCenter.y, mappedPoint1Z));
    const auto mappedVertexB = ToHkVector(RE::NiPoint3(mappedCenter.x, mappedCenter.y, mappedPoint2Z));
    const auto previousRadius = worldCapsuleShape->radius;
    const auto previousHeight = worldCapsuleShape->vertexA.GetDistance3(worldCapsuleShape->vertexB) + 2.0 * previousRadius;
    worldCapsuleShape->radius = mappedRadius;
    worldCapsuleShape->vertexA = mappedVertexA;
    worldCapsuleShape->vertexB = mappedVertexB;
    lastActorState.sittingPoseApplied = a_poseFlags.isSitting;
    lastActorState.sneakingPoseApplied = isPlayerSneaking || isNPCSneaking;

    const auto convexRebuild = a_rebuildConvex && SetConvexShape(a_actor, context.controller, mappedRadius, mappedPoint1Z, mappedPoint2Z, a_data.bump.translation, a_name, a_log);

    if (a_log) {
        logger::info(
            "\n"
            "  Actor State    : {}, \n"
            "  Center         : ({}, {}, {}) -> ({}, {}, {}), \n"
            "  Default Z      : top {}, bottom {}, center {}, \n"
            "  Anchor  Z      : {}, \n"
            "  Mapped  Z      : top {}, bottom {}, center {}, \n"
            "  Radius         : {} -> {}, \n"
            "  Height         : {} -> {}, \n"
            "  Convex Rebuild : {}",
            a_poseFlags.isSitting ? "Sitting" : "Standing",
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
            mappedHeight,
            convexRebuild
        );
    }
    return true;
}

//apply radius and xy collision only
bool Manager::SetCameraCollisionData(const VCD::CollisionData& a_data)
{
    auto* playerCamera = RE::PlayerCamera::GetSingleton();
    if (!playerCamera) {
        return false;
    }

    auto& cameraRTD = playerCamera->GetRuntimeData();
    if (!cameraRTD.unk120) {
        return false;
    }

    //set globals that are used in ThirdPersonState_SetRotation hook 
    // applying changes here results in the changes being overwritten next frame
    auto Apply = [&]()
    {
       cameraGlobals::CollisionPosX = a_data.bump.translation.x;
       cameraGlobals::CollisionPosY = a_data.bump.translation.y;
    };
  
    Apply(); 

    //I still dont know when unk08 is used its not used at all normally
    //Apply(cameraRTD.unk120->unk08.get());

    Dynamics::ApplyCameraCollisionRadius(a_data.capsule.radius);

    return true;
}

bool Manager::SetConvexShape(const RE::Actor* a_actor, 
    RE::bhkCharacterController* a_controller, 
    const float& a_radius, const float& a_point1Z, const float& a_point2Z, 
    const RE::NiPoint3& a_translation, const char* a_name, const bool& a_log)
{
    if (!a_actor || !a_controller) {
        return false;
    }

    ConvexShapeData convex{};
    if (!FindControllerConvexShape(a_controller, convex) || !convex.convexShape) {
        if (a_log) {
            logger::warn("Actor convex rebuild skipped [{}]: convex controller shape unavailable", a_name ? a_name : "Unknown");
        }
        return false;
    }

    const auto formID = a_actor->GetFormID();
    auto& state = convexShapeStates[formID];
    if (state.controller != a_controller || (state.currentShape && state.currentShape != convex.convexShape)) {
        state = {};
    }
    if (!CacheConvexShapeState(formID, a_controller, convex.convexShape, state)) {
        return false;
    }

    if (state.originalVertices.size() != 18) {
        if (a_log) {
            logger::warn("Actor convex rebuild skipped [{}]: unsupported vertex count {}", a_name ? a_name : "Unknown", state.originalVertices.size());
        }
        return false;
    }

    const auto* refPresetConfig = GetDefaultPresetConfig(Preset::kVanilla);
    const auto refRadius = refPresetConfig ? refPresetConfig->data.capsule.radius : a_radius;
    const auto refHeight = refPresetConfig ? std::abs(refPresetConfig->data.capsule.point1.z - refPresetConfig->data.capsule.point2.z) : std::abs(a_point1Z - a_point2Z);
    const auto refForward = refPresetConfig ? refPresetConfig->data.bump.translation.y : 0.0F;
    const auto refSide = refPresetConfig ? refPresetConfig->data.bump.translation.x : 0.0F;
    const auto presetHeight = std::abs(a_point1Z - a_point2Z);
    const auto radiusMult = refRadius > 0.0F ? a_radius / refRadius : 1.0F;
    const auto heightMult = refHeight > 0.0F ? presetHeight / refHeight : 1.0F;
    const auto forwardOffset = (a_translation.y - refForward) * RE::bhkWorld::GetWorldScale();
    const auto sideOffset = (a_translation.x - refSide) * RE::bhkWorld::GetWorldScale();

    auto newVertices = state.originalVertices;
    const auto topVertex = state.originalVertices[9];
    const auto bottomVertex = state.originalVertices[8];
    const auto topDelta = topVertex - bottomVertex;
    const auto newTopVertex = bottomVertex + (topDelta * RE::hkVector4(heightMult));
    const auto topZDelta = newTopVertex.quad.m128_f32[2] - topVertex.quad.m128_f32[2];

    newVertices[9] = newTopVertex;
    for (const auto& index : { 1, 3, 4, 5, 7, 11, 13, 16 }) {
        newVertices[index].quad.m128_f32[2] += topZDelta;
    }

    for (const auto& index : { 1, 3, 4, 5, 7, 11, 13, 16, 0, 2, 6, 10, 12, 14, 15, 17 }) {
        auto vertex = ToNiPoint3(newVertices[index]);
        auto ringVertex = vertex;
        ringVertex.z = 0.0F;
        if (ringVertex.Unitize()) {
            ringVertex *= state.originalRadius * radiusMult;
            ringVertex.z = vertex.z;
            newVertices[index] = ToHkVector(ringVertex);
        }
    }

    for (auto& vertex : newVertices) {
        vertex.quad.m128_f32[0] += sideOffset;
        vertex.quad.m128_f32[1] += forwardOffset;
    }

    Havok::StridedVertices stridedVertices(newVertices.data(), static_cast<int>(newVertices.size()));
    Havok::ConvexVerticesBuildConfig buildConfig{ false, false, true, 0.05F, 0, 0.0F, 0.0F, -0.1F };
    auto* newShape = Havok::AllocateConvexVerticesShape();
    if (!newShape) {
        if (a_log) {
            logger::warn("Actor convex rebuild skipped [{}]: allocation failed", a_name ? a_name : "Unknown");
        }
        return false;
    }

    Havok::ConvexVerticesShapeCtor(newShape, stridedVertices, buildConfig);
    reinterpret_cast<std::uintptr_t*>(newShape)[0] = RE::VTABLE_hkCharControllerShape[0].address();

    if (!ReplaceControllerConvexShape(a_controller, convex, newShape)) {
        newShape->RemoveReference();
        if (a_log) {
            logger::warn("Actor convex rebuild skipped [{}]: replacement failed", a_name ? a_name : "Unknown");
        }
        return false;
    }
    state.currentShape = newShape;

    if (a_log) {
        logger::debug("Actor convex rebuild [{}]: vertices={}, radius mult={}, height mult={}, offset=({}, {})",
            a_name ? a_name : "Unknown", 
            newVertices.size(), 
            radiusMult, 
            heightMult, 
            sideOffset,
            forwardOffset);
    }

    return true;
}
