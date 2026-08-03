#pragma once

#include "plugin.hpp"

#include <array>
#include <chrono>

namespace StepConstraints
{
	inline constexpr float kMinimumStepHeight = 0.25F;
	inline constexpr float kMaximumStepHeight = 40.0F;
	inline constexpr float kMinimumGroundNormalZ = 0.7F;
	inline constexpr float kMinimumStepContactNormalZ = 0.25F;
	inline constexpr float kMaximumStepContactNormalZ = 0.7F;
	inline constexpr float kMinimumMovementIntoObstacle = 0.1F;
	inline constexpr float kConstraintNormalMatch = 0.995F;
	inline constexpr std::chrono::milliseconds kGroundHeightCacheLifetime{ 500 };
	inline constexpr std::size_t kMaximumGroundHeightCaches = 65;

	struct GroundHeightCache
	{
		const RE::bhkCharProxyController* controller{ nullptr };
		float height{ 0.0F };
		std::chrono::steady_clock::time_point updatedAt{};
	};

	inline std::array<GroundHeightCache, kMaximumGroundHeightCaches>& GetGroundHeightCaches()
	{
		static std::array<GroundHeightCache, kMaximumGroundHeightCaches> caches{};
		return caches;
	}

	inline float GetHorizontalLengthSquared(const float a_x, const float a_y)
	{
		return (a_x * a_x) + (a_y * a_y);
	}

	inline float GetHorizontalLengthSquared(const RE::hkVector4& a_vector)
	{
		return GetHorizontalLengthSquared(a_vector.quad.m128_f32[0], a_vector.quad.m128_f32[1]);
	}

	inline float GetHorizontalDot(const RE::hkVector4& a_left, const RE::hkVector4& a_right)
	{
		return (a_left.quad.m128_f32[0] * a_right.quad.m128_f32[0]) +
			   (a_left.quad.m128_f32[1] * a_right.quad.m128_f32[1]);
	}

	inline float GetHorizontalDirectionMatch(const RE::hkVector4& a_left, const RE::hkVector4& a_right, const float a_leftLength, const float a_rightLength)
	{
		return GetHorizontalDot(a_left, a_right) / (a_leftLength * a_rightLength);
	}

	inline GroundHeightCache* FindGroundHeightCache(const RE::bhkCharProxyController* a_controller)
	{
		for (auto& cache : GetGroundHeightCaches()) {
			if (cache.controller == a_controller) {
				return &cache;
			}
		}

		return nullptr;
	}

	inline void InvalidateGroundHeightCache(const RE::bhkCharProxyController* a_controller)
	{
		if (auto* cache = FindGroundHeightCache(a_controller)) {
			*cache = {};
		}
	}

	inline bool IsStaticStepSurface(const RE::hkpCollidable* a_collidable)
	{
		return a_collidable && a_collidable->GetCollisionLayer() == RE::COL_LAYER::kStatic;
	}

	GroundHeightCache& GetGroundHeightCache(const RE::bhkCharProxyController* a_controller);

	void UpdateGroundHeightCache(const RE::bhkCharProxyController* a_controller, float a_groundHeight);

	bool GetCachedGroundHeight(const RE::bhkCharProxyController* a_controller, float& a_groundHeight);

	RE::MATERIAL_ID GetContactMaterial(const RE::hkpCollidable* a_collidable, const RE::hkpShapeKey& a_shapeKey);

	bool IsManagedCharacter(const RE::bhkCharProxyController* a_controller);

	bool Fix(
		RE::bhkCharProxyController* a_controller,
		const RE::hkpCharacterProxy* a_proxy,
		const RE::hkArray<RE::hkpRootCdPoint>& a_manifold,
		RE::hkpSimplexSolverInput& a_input,
		std::int32_t a_constraintCountBefore);
}
