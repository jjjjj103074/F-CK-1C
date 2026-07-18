#pragma once

#include "../Common/Clamp.h"
#include "../Common/Vec3.h"
#include <cmath>

namespace Systems
{
static const int kSuspensionWheelCount = 3;

struct SuspensionSystemConfig
{
	Common::Vec3 fallback_gear_points[kSuspensionWheelCount] = {
		Common::Vec3(4.12, -1.912, 0.0),
		Common::Vec3(-1.185, -1.913, -0.7905),
		Common::Vec3(-1.185, -1.913, 0.7905)
	};

	double fallback_wheel_radius[kSuspensionWheelCount] = { 0.2286, 0.3048, 0.3048 };
	double fallback_spring[kSuspensionWheelCount] = { 1000000.0, 3200000.0, 3200000.0 };
	double fallback_damping[kSuspensionWheelCount] = { 12000.0, 20000.0, 20000.0 };
	double fallback_contact_band[kSuspensionWheelCount] = { 0.015, 0.055, 0.055 };
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

	double fallback_compression[kSuspensionWheelCount] = { 0.0, 0.0, 0.0 };
	double fallback_force_mag[kSuspensionWheelCount] = { 0.0, 0.0, 0.0 };
	bool fallback_wow[kSuspensionWheelCount] = { false, false, false };
	double fallback_ground_force = 0.0;
};

struct SuspensionFallbackInput
{
	double altitude_agl = 0.0;
	double pitch = 0.0;
	double roll = 0.0;
	double velocity_world_y = 0.0;
	double velocity_body_x = 0.0;
	double gear_pos = 0.0;
	double current_mass = 0.0;
	double left_throttle_input = 0.0;
	double right_throttle_input = 0.0;
	double left_thrust_force = 0.0;
	double right_thrust_force = 0.0;
	double wheel_brake_left = 0.0;
	double wheel_brake_right = 0.0;
};

struct SuspensionFeedback
{
	int index = 0;
	double compression = 0.0;
	Common::Vec3 force;
};

struct SuspensionFallbackContext
{
	const SuspensionSystemConfig& config;
	const SuspensionFallbackInput& input;
	double sink_rate = 0.0;
	double gear_support = 0.0;
};

struct FallbackGearLoads
{
	double total_force = 0.0;
	double total_main_normal = 0.0;
	double left_main_normal = 0.0;
	double right_main_normal = 0.0;
	bool gear_contact = false;
};

inline bool valid_suspension_index(int idx)
{
	return idx >= 0 && idx < kSuspensionWheelCount;
}

inline bool has_suspension_feedback(const SuspensionSystemState& state)
{
	return state.feedback_valid[0] || state.feedback_valid[1] || state.feedback_valid[2];
}

inline bool any_wow(const SuspensionSystemState& state)
{
	return state.wow[0] || state.wow[1] || state.wow[2];
}

inline Common::Vec3 active_susp_wheel_pos(
	const SuspensionSystemConfig& config,
	int idx,
	double wheel_y_offset)
{
	if (!valid_suspension_index(idx))
	{
		return Common::Vec3();
	}

	Common::Vec3 pos = config.fallback_gear_points[idx];
	pos.y += wheel_y_offset;
	return pos;
}

inline double fallback_world_vertical_offset(
	const Common::Vec3& point,
	double pitch,
	double roll)
{
	const double cp = std::cos(pitch);
	const double sp = std::sin(pitch);
	const double cr = std::cos(roll);
	const double sr = std::sin(roll);

	const double y_after_pitch = point.x * sp + point.y * cp;
	return (y_after_pitch * cr) - (point.z * sr);
}

inline void clear_fallback_state(SuspensionSystemState& state)
{
	for (int i = 0; i < kSuspensionWheelCount; ++i)
	{
		state.fallback_compression[i] = 0.0;
		state.fallback_force_mag[i] = 0.0;
		state.fallback_wow[i] = false;
	}
}

inline void reset_suspension_feedback_state(SuspensionSystemState& state)
{
	for (int i = 0; i < kSuspensionWheelCount; ++i)
	{
		state.compression[i] = 0.0;
		state.force_vec[i] = Common::Vec3();
		state.force_mag[i] = 0.0;
		state.wow[i] = false;
		state.feedback_valid[i] = false;
		state.fallback_compression[i] = 0.0;
		state.fallback_force_mag[i] = 0.0;
		state.fallback_wow[i] = false;
	}
	state.fallback_ground_force = 0.0;
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
	state.wow[index] = (state.compression[index] > 1e-4) ||
		(state.force_mag[index] > 50.0);
	return true;
}

inline bool update_on_ground(SuspensionSystemState& state, double gear_pos)
{
	state.on_ground = (gear_pos > 0.5) && has_suspension_feedback(state) && any_wow(state);
	return state.on_ground;
}

inline SuspensionFallbackContext make_suspension_fallback_context(
	const SuspensionSystemConfig& config,
	const SuspensionFallbackInput& input)
{
	return {
		config,
		input,
		Common::limit(-input.velocity_world_y, 0.0, 80.0),
		Common::limit((input.gear_pos - 0.2) / 0.8, 0.0, 1.0)
	};
}

inline double fallback_wheel_force(
	const SuspensionFallbackContext& context,
	int index,
	double compression)
{
	double force = compression * context.config.fallback_spring[index] *
		context.gear_support;
	force += context.sink_rate * context.config.fallback_damping[index] *
		context.gear_support;
	const double weight = context.input.current_mass * 9.81;
	const double force_limit = index == 0 ? weight * 0.45 : weight * 1.15;
	return Common::limit(force, 0.0, force_limit);
}

template <typename AddLocalForce>
inline FallbackGearLoads apply_fallback_wheel_contacts(
	SuspensionSystemState& state,
	const SuspensionFallbackContext& context,
	AddLocalForce& add_local_force)
{
	FallbackGearLoads loads;
	if (context.gear_support <= 0.0)
	{
		return loads;
	}
	for (int index = 0; index < kSuspensionWheelCount; ++index)
	{
		const double wheel_bottom_agl = context.input.altitude_agl +
			fallback_world_vertical_offset(
				context.config.fallback_gear_points[index],
				context.input.pitch,
				context.input.roll) - context.config.fallback_wheel_radius[index];
		const double compression =
			context.config.fallback_contact_band[index] - wheel_bottom_agl;
		if (compression <= 0.0)
		{
			continue;
		}
		const double force = fallback_wheel_force(context, index, compression);
		add_local_force(
			Common::Vec3(0.0, force, 0.0), context.config.fallback_gear_points[index]);
		state.fallback_compression[index] = compression;
		state.fallback_force_mag[index] = force;
		state.fallback_wow[index] = true;
		loads.total_force += force;
		loads.gear_contact = true;
		if (index == 1) loads.left_main_normal = force;
		if (index == 2) loads.right_main_normal = force;
	}
	loads.total_main_normal = loads.left_main_normal + loads.right_main_normal;
	return loads;
}

template <typename AddLocalForce>
inline void apply_fallback_longitudinal_resistance(
	const FallbackGearLoads& loads,
	const SuspensionFallbackContext& context,
	AddLocalForce& add_local_force)
{
	if (loads.total_main_normal <= 1.0)
	{
		return;
	}
	const SuspensionFallbackInput& input = context.input;
	const double forward_speed = input.velocity_body_x;
	const double speed_abs = std::fabs(forward_speed);
	const double speed_sign = forward_speed >= 0.0 ? 1.0 : -1.0;
	const double avg_throttle = 0.5 *
		(input.left_throttle_input + input.right_throttle_input);
	const double brake_norm =
		loads.left_main_normal * Common::limit(input.wheel_brake_left, 0.0, 1.0) +
		loads.right_main_normal * Common::limit(input.wheel_brake_right, 0.0, 1.0);
	double resistance = loads.total_main_normal * 0.035 + brake_norm * 0.85;
	if (speed_abs < 1.5 && avg_throttle < 0.05)
	{
		resistance += Common::limit(
			input.left_thrust_force + input.right_thrust_force,
			0.0,
			loads.total_main_normal * 0.12);
	}
	if (speed_abs > 0.05)
	{
		add_local_force(
			Common::Vec3(-speed_sign * resistance, 0.0, 0.0),
			Common::Vec3(-0.9, -1.6, 0.0));
	}
	else if (avg_throttle < 0.05 || brake_norm > 1.0)
	{
		add_local_force(
			Common::Vec3(-resistance, 0.0, 0.0),
			Common::Vec3(-0.9, -1.6, 0.0));
	}
}

template <typename AddLocalForce>
inline double apply_fallback_belly_force(
	bool gear_contact,
	const SuspensionFallbackContext& context,
	AddLocalForce& add_local_force)
{
	if (gear_contact)
	{
		return 0.0;
	}
	const double belly_bottom_agl = context.input.altitude_agl +
		fallback_world_vertical_offset(
			context.config.fallback_belly_point,
			context.input.pitch,
			context.input.roll);
	const double compression = 0.03 - belly_bottom_agl;
	if (compression <= 0.0)
	{
		return 0.0;
	}
	double force = compression * 260000.0 + context.sink_rate * 40000.0;
	force = Common::limit(force, 0.0, context.input.current_mass * 9.81 * 2.0);
	add_local_force(
		Common::Vec3(0.0, force, 0.0), context.config.fallback_belly_point);
	return force;
}

template <typename AddLocalForce>
inline double apply_fallback_ground_forces(
	SuspensionSystemState& state,
	const SuspensionFallbackContext& context,
	AddLocalForce add_local_force)
{
	clear_fallback_state(state);
	if (!context.config.enable_fallback_ground_forces)
	{
		state.fallback_ground_force = 0.0;
		return 0.0;
	}
	FallbackGearLoads loads = apply_fallback_wheel_contacts(
		state, context, add_local_force);
	apply_fallback_longitudinal_resistance(loads, context, add_local_force);
	loads.total_force += apply_fallback_belly_force(
		loads.gear_contact, context, add_local_force);
	state.fallback_ground_force = loads.total_force;
	return loads.total_force;
}
}
