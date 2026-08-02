#include "step.hpp"

#include "dynamics.hpp"
#include "helper.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
	constexpr float kMinimumStepHeight = 0.25F;
	constexpr float kMaximumStepHeight = 40.0F;
	constexpr float kMinimumGroundNormalZ = 0.7F;
	constexpr float kMinimumStepContactNormalZ = 0.25F;
	constexpr float kMaximumStepContactNormalZ = 0.7F;
	constexpr float kMinimumMovementIntoObstacle = 0.1F;
	constexpr float kConstraintNormalMatch = 0.995F;

	RE::MATERIAL_ID GetContactMaterial(
		const RE::hkpCollidable* a_collidable,
		const RE::hkpShapeKey& a_shapeKey)
	{
		if (!a_collidable) {
			return RE::MATERIAL_ID::kNone;
		}

		const auto* shape = a_collidable->GetShape();
		if (!shape || !shape->userData) {
			return RE::MATERIAL_ID::kNone;
		}

		if (a_shapeKey == RE::HK_INVALID_SHAPE_KEY) {
			return shape->userData->materialID;
		}

		return shape->userData->GetMaterialID(a_shapeKey);
	}

	bool IsStaticStepSurface(const RE::hkpCollidable* a_collidable)
	{
		return a_collidable && a_collidable->GetCollisionLayer() == RE::COL_LAYER::kStatic;
	}
}

bool StepConstraints::IsManagedCharacter(const RE::bhkCharProxyController* a_controller)
{
	if (!a_controller) {
		return false;
	}

	const auto* characterController = static_cast<const RE::bhkCharacterController*>(a_controller);
	auto* player = RE::PlayerCharacter::GetSingleton();
	if (player && player->GetCharController() == characterController) {
		return true;
	}

	auto& npcState = Dynamics::GetNPCDynamicsState();
	for (auto& actorState : npcState.actors) {
		if (actorState.controller != characterController) {
			continue;
		}

		auto actorPtr = actorState.actor.get();
		auto* actor = actorPtr.get();
		if (actor && actor->GetCharController() == characterController) {
			return true;
		}
	}

	return false;
}

bool StepConstraints::Fix(
	RE::bhkCharProxyController* a_controller,
	const RE::hkpCharacterProxy* a_proxy,
	const RE::hkArray<RE::hkpRootCdPoint>& a_manifold,
	RE::hkpSimplexSolverInput& a_input,
	const std::int32_t a_constraintCountBefore)
{
	// Only correct a supported character after vanilla appended a new blocking constraint.
	if (!a_controller || !a_proxy || !a_proxy->shapePhantom || !a_input.constraints ||
		a_controller->flags.any(RE::CHARACTER_FLAGS::kTryStep) ||
		a_controller->surfaceInfo.supportedState.underlying() !=
			static_cast<std::uint32_t>(RE::hkpSurfaceInfo::SupportedState::kSupported) ||
		a_constraintCountBefore <= 0 || a_input.numConstraints <= a_constraintCountBefore) {
		return false;
	}

	// Ignore nearly vertical movement.
	if (StepConstraints::GetHorizontalLengthSquared(a_input.velocity) <= 0.01F) {
		return false;
	}


	const auto worldScale = RE::bhkWorld::GetWorldScale();
	if (!std::isfinite(worldScale) || worldScale <= 0.0F) {
		return false;
	}


	const auto* characterCollidable = a_proxy->shapePhantom->GetCollidable();
	if (!characterCollidable) {
		return false;
	}

	// Use the lowest static bumper contact as the current ground height.
	auto groundHeight = std::numeric_limits<float>::max();
	for (std::int32_t index = 0; index < a_manifold.size(); ++index) {
		const auto& point = a_manifold[index];
		if (point.rootCollidableA != characterCollidable || !IsStaticStepSurface(point.rootCollidableB)) {
			continue;
		}

		const auto characterMaterial = GetContactMaterial(characterCollidable, point.shapeKeyA);
		const auto normalZ = point.contact.separatingNormal.quad.m128_f32[2];
		if (VCD::IsCharacterBumperMaterial(characterMaterial) && normalZ >= kMinimumGroundNormalZ) {
			groundHeight = std::min(groundHeight, point.contact.position.quad.m128_f32[2]);
		}
	}

	if (groundHeight == std::numeric_limits<float>::max()) {
		return false;
	}

	// Track the strongest valid step contact and its matching solver constraints.
	const auto minimumStepHeight = kMinimumStepHeight * worldScale;
	const auto maximumStepHeight = kMaximumStepHeight * worldScale;
	auto bestMovementDot = -kMinimumMovementIntoObstacle;
	auto selectedContactIndex = -1;
	auto selectedConstraintIndex = -1;
	auto selectedBlockerIndex = -1;

	// Search static contacts for an obstacle that is low and sloped enough to step onto.
	// For example small stairs leading to inner doors.
	for (std::int32_t contactIndex = 0; contactIndex < a_manifold.size(); ++contactIndex) {
		const auto& point = a_manifold[contactIndex];
		if (point.rootCollidableA != characterCollidable || !IsStaticStepSurface(point.rootCollidableB)) {
			continue;
		}

		const auto& normal = point.contact.separatingNormal;
		const auto normalZ = normal.quad.m128_f32[2];
		const auto horizontalNormalLengthSquared = StepConstraints::GetHorizontalLengthSquared(normal);
		if (normalZ < kMinimumStepContactNormalZ || normalZ >= kMaximumStepContactNormalZ ||
			horizontalNormalLengthSquared <= 0.01F) {
			continue;
		}

		// Reject contacts outside Skyrim's usable step-height range.
		const auto stepHeight = point.contact.position.quad.m128_f32[2] - groundHeight;
		if (stepHeight < minimumStepHeight || stepHeight > maximumStepHeight) {
			continue;
		}


		const auto movementDot = StepConstraints::GetHorizontalDot(a_input.velocity, normal);
		if (movementDot >= bestMovementDot) {
			continue;
		}

		auto matchingConstraintIndex = -1;
		auto bestConstraintDot = kConstraintNormalMatch;
		for (std::int32_t constraintIndex = 0; constraintIndex < a_constraintCountBefore; ++constraintIndex) {
			const auto constraintDot = normal.Dot3(a_input.constraints[constraintIndex].plane);
			if (constraintDot > bestConstraintDot) {
				bestConstraintDot = constraintDot;
				matchingConstraintIndex = constraintIndex;
			}
		}
		if (matchingConstraintIndex < 0) {
			continue;
		}

		// Find the horizontal blocker vanilla appended for the same obstacle.
		const auto horizontalNormalLength = std::sqrt(horizontalNormalLengthSquared);
		auto matchingBlockerIndex = -1;
		for (std::int32_t blockerIndex = a_constraintCountBefore; blockerIndex < a_input.numConstraints; ++blockerIndex) {
			const auto& blockerPlane = a_input.constraints[blockerIndex].plane;
			const auto blockerHorizontalLength = std::sqrt(StepConstraints::GetHorizontalLengthSquared(blockerPlane));
			if (std::abs(blockerPlane.quad.m128_f32[2]) > 0.001F || blockerHorizontalLength <= 0.01F) {
				continue;
			}

			const auto blockerMatch = StepConstraints::GetHorizontalDirectionMatch(normal, blockerPlane, horizontalNormalLength, blockerHorizontalLength);
			if (blockerMatch >= kConstraintNormalMatch) {
				matchingBlockerIndex = blockerIndex;
				break;
			}
		}
		if (matchingBlockerIndex < 0) {
			continue;
		}

		// Keep the best fully matched contact for correction after the scan.
		bestMovementDot = movementDot;
		selectedContactIndex = contactIndex;
		selectedConstraintIndex = matchingConstraintIndex;
		selectedBlockerIndex = matchingBlockerIndex;
	}

	// Leave vanilla's solver result unchanged unless every required match was found.
	if (selectedContactIndex < 0 || selectedConstraintIndex < 0 || selectedBlockerIndex < 0) {
		return false;
	}

	// Tilt the matched contact plane upward and remove friction so vanilla can step over it.
	// This what Skyrim normally does dealing with stairs/steps based on debugging values.
	auto& stepConstraint = a_input.constraints[selectedConstraintIndex];
	const auto planeX = stepConstraint.plane.quad.m128_f32[0];
	const auto planeY = stepConstraint.plane.quad.m128_f32[1];
	const auto inverseLength = 1.0F / std::sqrt(StepConstraints::GetHorizontalLengthSquared(stepConstraint.plane) + 1.0F);
	stepConstraint.plane.quad.m128_f32[0] = planeX * inverseLength;
	stepConstraint.plane.quad.m128_f32[1] = planeY * inverseLength;
	stepConstraint.plane.quad.m128_f32[2] = inverseLength;
	stepConstraint.plane.quad.m128_f32[3] *= inverseLength;
	stepConstraint.staticFriction = 0.0F;
	stepConstraint.dynamicFriction = 0.0F;
	stepConstraint.extraUpStaticFriction = 0.0F;
	stepConstraint.extraDownStaticFriction = 0.0F;

	// Remove the redundant horizontal blocker while preserving later constraints.
	for (std::int32_t index = selectedBlockerIndex; index < a_input.numConstraints - 1; ++index) {
		a_input.constraints[index] = a_input.constraints[index + 1];
	}
	--a_input.numConstraints;

	// Hand the corrected constraints back to Skyrim's native step-up path.
	a_controller->flags.set(RE::CHARACTER_FLAGS::kTryStep);
	return true;
}
