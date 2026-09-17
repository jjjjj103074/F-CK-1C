#pragma once

#include "../../../Common/Vec3.h"
#include "../../Contracts/FrameContracts.h"

#include <array>
#include <cmath>

namespace Core
{
namespace Systems
{
inline constexpr double kWeightOnWheelCompressionThresholdM = 1e-4;
inline constexpr double kWeightOnWheelForceThresholdN = 50.0;
inline constexpr double kDeployedGearThresholdNormalized = 0.5;

struct SuspensionFeedbackState
{
	bool on_ground = false;
	std::array<double, kFrameSuspensionWheelCount> compression_m = {};
	std::array<Common::Vec3, kFrameSuspensionWheelCount> force_body_n = {};
	std::array<double, kFrameSuspensionWheelCount> force_magnitude_n = {};
	std::array<bool, kFrameSuspensionWheelCount> weight_on_wheel = {};
	std::array<bool, kFrameSuspensionWheelCount> feedback_valid = {};
};

struct SuspensionFeedbackSample
{
	int index = 0;
	double compression_m = 0.0;
	Common::Vec3 force_body_n;
};

inline bool valid_suspension_index(int index)
{
	return index >= 0 &&
		index < static_cast<int>(kFrameSuspensionWheelCount);
}

inline bool any_weight_on_wheels(const SuspensionFeedbackState& state)
{
	for (const bool weight_on_wheel : state.weight_on_wheel)
	{
		if (weight_on_wheel)
		{
			return true;
		}
	}
	return false;
}

inline bool update_suspension_feedback(
	SuspensionFeedbackState& state,
	const SuspensionFeedbackSample& sample)
{
	if (!valid_suspension_index(sample.index))
	{
		return false;
	}
	const std::size_t index = static_cast<std::size_t>(sample.index);
	state.feedback_valid[index] = true;
	state.compression_m[index] = sample.compression_m;
	state.force_body_n[index] = sample.force_body_n;
	state.force_magnitude_n[index] = std::sqrt(
		sample.force_body_n.x * sample.force_body_n.x +
		sample.force_body_n.y * sample.force_body_n.y +
		sample.force_body_n.z * sample.force_body_n.z);
	state.weight_on_wheel[index] =
		state.compression_m[index] >
			kWeightOnWheelCompressionThresholdM ||
		state.force_magnitude_n[index] > kWeightOnWheelForceThresholdN;
	return true;
}

inline void update_on_ground(
	SuspensionFeedbackState& state,
	double gear_position_normalized)
{
	state.on_ground =
		gear_position_normalized > kDeployedGearThresholdNormalized &&
		any_weight_on_wheels(state);
}
}
}
