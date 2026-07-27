#pragma once

#include "../../../Common/Vec3.h"
#include "../../Contracts/FrameContracts.h"

#include <array>
#include <cmath>

namespace Core
{
namespace Systems
{
inline constexpr double kWeightOnWheelCompressionThreshold = 1e-4;
inline constexpr double kWeightOnWheelForceThreshold = 50.0;
inline constexpr double kDeployedGearThreshold = 0.5;

struct SuspensionFeedbackState
{
	bool on_ground = false;
	std::array<double, kFrameSuspensionWheelCount> compression = {};
	std::array<Common::Vec3, kFrameSuspensionWheelCount> force = {};
	std::array<double, kFrameSuspensionWheelCount> force_magnitude = {};
	std::array<bool, kFrameSuspensionWheelCount> weight_on_wheel = {};
	std::array<bool, kFrameSuspensionWheelCount> feedback_valid = {};
};

struct SuspensionFeedbackSample
{
	int index = 0;
	double compression = 0.0;
	Common::Vec3 force;
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
	state.compression[index] = sample.compression;
	state.force[index] = sample.force;
	state.force_magnitude[index] = std::sqrt(
		sample.force.x * sample.force.x +
		sample.force.y * sample.force.y +
		sample.force.z * sample.force.z);
	state.weight_on_wheel[index] =
		state.compression[index] >
			kWeightOnWheelCompressionThreshold ||
		state.force_magnitude[index] > kWeightOnWheelForceThreshold;
	return true;
}

inline void update_on_ground(
	SuspensionFeedbackState& state,
	double gear_position)
{
	state.on_ground = gear_position > kDeployedGearThreshold &&
		any_weight_on_wheels(state);
}
}
}
