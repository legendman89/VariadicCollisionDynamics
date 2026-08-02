#pragma once

#include "plugin.hpp"

namespace StepConstraints
{
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

	inline float GetHorizontalDirectionMatch(
		const RE::hkVector4& a_left,
		const RE::hkVector4& a_right,
		const float a_leftLength,
		const float a_rightLength)
	{
		return GetHorizontalDot(a_left, a_right) / (a_leftLength * a_rightLength);
	}

	bool IsManagedCharacter(const RE::bhkCharProxyController* a_controller);

	bool Fix(
		RE::bhkCharProxyController* a_controller,
		const RE::hkpCharacterProxy* a_proxy,
		const RE::hkArray<RE::hkpRootCdPoint>& a_manifold,
		RE::hkpSimplexSolverInput& a_input,
		std::int32_t a_constraintCountBefore);
}
