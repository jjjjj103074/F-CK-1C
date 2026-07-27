#pragma once

#include "../Common/Vec3.h"

#include <cmath>

namespace Systems
{
inline constexpr int kSuspensionWheelCount = 3;
inline constexpr double kWeightOnWheelCompressionThreshold = 1e-4;
inline constexpr double kWeightOnWheelForceThreshold = 50.0;
inline constexpr double kDeployedGearThreshold = 0.5;

struct SuspensionSystemConfig
{
	Common::Vec3 fallback_gear_points[kSuspensionWheelCount] = {
		Common::Vec3(4.12, -1.912, 0.0),
		Common::Vec3(-1.185, -1.913, -0.7905),
		Common::Vec3(-1.185, -1.913, 0.7905)
	};

	double fallback_wheel_radius[kSuspensionWheelCount] = {
		0.2286, 0.3048, 0.3048
	};
	double fallback_spring[kSuspensionWheelCount] = {
		1000000.0, 3200000.0, 3200000.0
	};
	double fallback_damping[kSuspensionWheelCount] = {
		12000.0, 20000.0, 20000.0
	};
	double fallback_contact_band[kSuspensionWheelCount] = {
		0.015, 0.055, 0.055
	};
	Common::Vec3 fallback_belly_point = Common::Vec3(0.0, -1.05, 0.0);
	bool enable_fallback_ground_forces = false;
};

struct SuspensionSystemState
{
	bool on_ground = false;
	double compression[kSuspensionWheelCount] = { 0.0, 0.0, 0.0 };
	Common::Vec3 force_vec[kSuspensionWheelCount] = {
		Common::Vec3(),
		Common::Vec3(),
		Common::Vec3()
	};
	double force_mag[kSuspensionWheelCount] = { 0.0, 0.0, 0.0 };
	bool wow[kSuspensionWheelCount] = { false, false, false };
	bool feedback_valid[kSuspensionWheelCount] = { false, false, false };
};

struct SuspensionFeedback
{
	int index = 0;
	double compression = 0.0;
	Common::Vec3 force;
};

inline bool valid_suspension_index(int index)
{
	return index >= 0 && index < kSuspensionWheelCount;
}

inline bool has_suspension_feedback(const SuspensionSystemState& state)
{
	return state.feedback_valid[0] ||
		state.feedback_valid[1] ||
		state.feedback_valid[2];
}

inline bool any_wow(const SuspensionSystemState& state)
{
	return state.wow[0] || state.wow[1] || state.wow[2];
}

inline void reset_suspension_feedback_state(
	SuspensionSystemState& state)
{
	for (int index = 0; index < kSuspensionWheelCount; ++index)
	{
		state.compression[index] = 0.0;
		state.force_vec[index] = Common::Vec3();
		state.force_mag[index] = 0.0;
		state.wow[index] = false;
		state.feedback_valid[index] = false;
	}
	state.on_ground = false;
}

inline bool update_suspension_feedback(
	SuspensionSystemState& state,
	const SuspensionFeedback& feedback)
{
	if (!valid_suspension_index(feedback.index))
	{
		return false;
	}
	const int index = feedback.index;
	state.feedback_valid[index] = true;
	state.compression[index] = feedback.compression;
	state.force_vec[index] = feedback.force;
	state.force_mag[index] = std::sqrt(
		feedback.force.x * feedback.force.x +
		feedback.force.y * feedback.force.y +
		feedback.force.z * feedback.force.z);
	state.wow[index] =
		state.compression[index] >
			kWeightOnWheelCompressionThreshold ||
		state.force_mag[index] > kWeightOnWheelForceThreshold;
	return true;
}

inline bool update_on_ground(
	SuspensionSystemState& state,
	double gear_position)
{
	state.on_ground =
		gear_position > kDeployedGearThreshold &&
		has_suspension_feedback(state) &&
		any_wow(state);
	return state.on_ground;
}
}
