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

	const char* active_collision_shell_name = "F-CK-1C-box.edm";
	const char* active_gear_shell_nodes = "F-CK-1C-F_W/F-CK-1C-LBW/F-CK-1C-RBW";
	const char* suspension_mode_name = "native_collision_shell_name";
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

inline bool any_fallback_wow(const SuspensionSystemState& state)
{
	return state.fallback_wow[0] || state.fallback_wow[1] || state.fallback_wow[2];
}

inline double suspension_total_force_mag(const SuspensionSystemState& state)
{
	return state.force_mag[0] + state.force_mag[1] + state.force_mag[2];
}

inline double suspension_visual_arg(const SuspensionSystemState& state, int idx, double gear_pos)
{
	if (!valid_suspension_index(idx) || !state.feedback_valid[idx])
	{
		return 0.0;
	}

	return Common::limit(state.compression[idx], 0.0, 1.0) * Common::limit(gear_pos, 0.0, 1.0);
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
	int idx,
	double compression,
	double force_x,
	double force_y,
	double force_z)
{
	if (!valid_suspension_index(idx))
	{
		return false;
	}

	state.feedback_valid[idx] = true;
	state.compression[idx] = compression;
	state.force_vec[idx] = Common::Vec3(force_x, force_y, force_z);
	state.force_mag[idx] = std::sqrt(force_x * force_x + force_y * force_y + force_z * force_z);
	state.wow[idx] = (state.compression[idx] > 1e-4) || (state.force_mag[idx] > 50.0);
	return true;
}

inline bool update_on_ground(SuspensionSystemState& state, double gear_pos)
{
	state.on_ground = (gear_pos > 0.5) && has_suspension_feedback(state) && any_wow(state);
	return state.on_ground;
}

template <typename AddLocalForce>
inline double apply_fallback_ground_forces(
	SuspensionSystemState& state,
	const SuspensionSystemConfig& config,
	const SuspensionFallbackInput& input,
	AddLocalForce add_local_force)
{
	clear_fallback_state(state);

	if (!config.enable_fallback_ground_forces)
	{
		state.fallback_ground_force = 0.0;
		return 0.0;
	}

	double total_force = 0.0;
	double total_main_normal = 0.0;
	double left_main_normal = 0.0;
	double right_main_normal = 0.0;
	const double sink_rate = Common::limit(-input.velocity_world_y, 0.0, 80.0);
	const double gear_support = Common::limit((input.gear_pos - 0.2) / 0.8, 0.0, 1.0);
	bool gear_contact = false;

	if (gear_support > 0.0)
	{
		for (int i = 0; i < kSuspensionWheelCount; ++i)
		{
			const double wheel_bottom_agl =
				input.altitude_agl +
				fallback_world_vertical_offset(config.fallback_gear_points[i], input.pitch, input.roll) -
				config.fallback_wheel_radius[i];
			const double compression = config.fallback_contact_band[i] - wheel_bottom_agl;

			if (compression > 0.0)
			{
				double force_mag =
					(compression * config.fallback_spring[i] * gear_support) +
					(sink_rate * config.fallback_damping[i] * gear_support);

				if (i == 0)
				{
					force_mag = Common::limit(force_mag, 0.0, input.current_mass * 9.81 * 0.45);
				}
				else
				{
					force_mag = Common::limit(force_mag, 0.0, input.current_mass * 9.81 * 1.15);
				}

				add_local_force(Common::Vec3(0.0, force_mag, 0.0), config.fallback_gear_points[i]);

				state.fallback_compression[i] = compression;
				state.fallback_force_mag[i] = force_mag;
				state.fallback_wow[i] = true;

				total_force += force_mag;
				if (i == 1)
				{
					left_main_normal = force_mag;
					total_main_normal += force_mag;
				}
				else if (i == 2)
				{
					right_main_normal = force_mag;
					total_main_normal += force_mag;
				}
				gear_contact = true;
			}
		}
	}

	if (total_main_normal > 1.0)
	{
		const double forward_speed = input.velocity_body_x;
		const double speed_abs = std::fabs(forward_speed);
		const double speed_sign = (forward_speed >= 0.0) ? 1.0 : -1.0;
		const double avg_throttle = 0.5 * (input.left_throttle_input + input.right_throttle_input);
		const double brake_norm =
			(left_main_normal * Common::limit(input.wheel_brake_left, 0.0, 1.0)) +
			(right_main_normal * Common::limit(input.wheel_brake_right, 0.0, 1.0));

		double longitudinal_resist =
			(total_main_normal * 0.035) +
			(brake_norm * 0.85);

		if (speed_abs < 1.5 && avg_throttle < 0.05)
		{
			longitudinal_resist += Common::limit(
				input.left_thrust_force + input.right_thrust_force,
				0.0,
				total_main_normal * 0.12);
		}

		if (speed_abs > 0.05)
		{
			add_local_force(Common::Vec3(-speed_sign * longitudinal_resist, 0.0, 0.0), Common::Vec3(-0.9, -1.6, 0.0));
		}
		else if (avg_throttle < 0.05 || brake_norm > 1.0)
		{
			add_local_force(Common::Vec3(-longitudinal_resist, 0.0, 0.0), Common::Vec3(-0.9, -1.6, 0.0));
		}
	}

	if (!gear_contact)
	{
		const double belly_bottom_agl =
			input.altitude_agl +
			fallback_world_vertical_offset(config.fallback_belly_point, input.pitch, input.roll);
		const double belly_compression = 0.03 - belly_bottom_agl;

		if (belly_compression > 0.0)
		{
			double belly_force =
				(belly_compression * 260000.0) +
				(sink_rate * 40000.0);

			belly_force = Common::limit(belly_force, 0.0, input.current_mass * 9.81 * 2.0);
			add_local_force(Common::Vec3(0.0, belly_force, 0.0), config.fallback_belly_point);
			total_force += belly_force;
		}
	}

	state.fallback_ground_force = total_force;
	return total_force;
}
}
