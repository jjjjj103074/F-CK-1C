#pragma once

#include "Common/Clamp.h"

#include <cmath>
#include <stdexcept>

namespace Systems
{
struct PrimaryAxisState
{
	double input = 0.0;
	int discrete = 0;
	bool analog = true;
	double trim = 0.0;
};

struct PrimaryControlState
{
	PrimaryAxisState pitch;
	PrimaryAxisState roll;
	PrimaryAxisState yaw;
};

struct ThrottleChannelState
{
	double axis_cmd = 0.0;
	double keyboard_cmd = 0.0;
	bool use_axis = false;
	double pilot_cmd = 0.0;
};

struct ThrottleInputState
{
	bool axis_inverted = true;
	ThrottleChannelState left;
	ThrottleChannelState right;
};

inline void reset_throttle_channel(ThrottleChannelState& channel, double command)
{
	channel.axis_cmd = command;
	channel.keyboard_cmd = command;
	channel.use_axis = false;
	channel.pilot_cmd = command;
}

inline void reset_throttle_inputs(ThrottleInputState& throttles, double left_command, double right_command)
{
	reset_throttle_channel(throttles.left, left_command);
	reset_throttle_channel(throttles.right, right_command);
}

inline void set_primary_axis_input(PrimaryAxisState& axis, double value)
{
	axis.input = Common::limit(value, -1.0, 1.0);
	axis.analog = true;
	axis.discrete = 0;
}

inline void set_primary_discrete_input(PrimaryAxisState& axis, int discrete)
{
	axis.discrete = discrete;
	axis.analog = false;
}

inline void adjust_primary_trim(PrimaryAxisState& axis, double delta)
{
	axis.trim += delta;
}

inline void reset_primary_trim(PrimaryAxisState& axis)
{
	axis.trim = 0.0;
}

inline void set_pitch_axis_input(PrimaryControlState& controls, double value)
{
	set_primary_axis_input(controls.pitch, value);
}

inline void set_roll_axis_input(PrimaryControlState& controls, double value)
{
	set_primary_axis_input(controls.roll, value);
}

inline void set_yaw_axis_input(PrimaryControlState& controls, double value)
{
	set_primary_axis_input(controls.yaw, value);
}

inline void set_pitch_discrete_input(PrimaryControlState& controls, int discrete)
{
	set_primary_discrete_input(controls.pitch, discrete);
}

inline void set_roll_discrete_input(PrimaryControlState& controls, int discrete)
{
	set_primary_discrete_input(controls.roll, discrete);
}

inline void set_yaw_discrete_input(PrimaryControlState& controls, int discrete)
{
	set_primary_discrete_input(controls.yaw, discrete);
}

inline void adjust_pitch_trim(PrimaryControlState& controls, double delta)
{
	adjust_primary_trim(controls.pitch, delta);
}

inline void adjust_roll_trim(PrimaryControlState& controls, double delta)
{
	adjust_primary_trim(controls.roll, delta);
}

inline void adjust_yaw_trim(PrimaryControlState& controls, double delta)
{
	adjust_primary_trim(controls.yaw, delta);
}

inline void reset_primary_trims(PrimaryControlState& controls)
{
	reset_primary_trim(controls.pitch);
	reset_primary_trim(controls.roll);
	reset_primary_trim(controls.yaw);
}

inline void reset_primary_commands(PrimaryControlState& controls)
{
	controls.pitch.input = 0.0;
	controls.pitch.trim = 0.0;
	controls.roll.input = 0.0;
	controls.roll.trim = 0.0;
	controls.yaw.input = 0.0;
	controls.yaw.trim = 0.0;
}

struct PrimaryAxisDynamics
{
	double command_rate_normalized_s = 0.0;
	double centering_retention_per_reference_tick = 1.0;
	double centering_positive_threshold = 0.0;
	double centering_negative_threshold = 0.0;
	bool center_only_outside_thresholds = false;
};

inline bool should_center_axis(
	const PrimaryAxisState& axis,
	const PrimaryAxisDynamics& dynamics)
{
	return axis.discrete == 0 &&
		(!dynamics.center_only_outside_thresholds ||
			axis.input > dynamics.centering_positive_threshold ||
			axis.input < dynamics.centering_negative_threshold);
}

inline PrimaryAxisState update_primary_axis(
	const PrimaryAxisState& current,
	const PrimaryAxisDynamics& dynamics,
	double dt_s)
{
	constexpr double kReferenceUpdateRateHz = 64.0;
	constexpr double kDiscreteCommandThreshold = 0.1;
	if (!std::isfinite(dt_s) || dt_s <= 0.0)
	{
		throw std::invalid_argument(
			"PilotControls axis dt must be positive and finite.");
	}
	PrimaryAxisState next = current;
	if (current.analog)
	{
		next.input = Common::limit(current.input, -1.0, 1.0);
		return next;
	}
	if (std::fabs(static_cast<double>(current.discrete)) >
		kDiscreteCommandThreshold)
	{
		next.input = Common::limit(
			current.input + current.discrete *
				dynamics.command_rate_normalized_s * dt_s,
			-1.0,
			1.0);
		return next;
	}
	if (should_center_axis(current, dynamics))
	{
		next.input *= std::pow(
			dynamics.centering_retention_per_reference_tick,
			dt_s * kReferenceUpdateRateHz);
	}
	return next;
}

inline PrimaryControlState update_primary_control_inputs(
	const PrimaryControlState& current,
	double dt_s);

inline double clamp_pitch_roll_trim(double trim)
{
	return Common::limit(trim, -0.3, 0.3);
}

inline double clamp_yaw_trim(double trim)
{
	return Common::limit(trim, -0.2, 0.2);
}

inline PrimaryControlState update_primary_control_inputs(
	const PrimaryControlState& current,
	double dt_s)
{
	// These rates and retention factors preserve the legacy 64 Hz keyboard
	// feel while making the result depend on elapsed simulation time.
	constexpr double kPitchYawCommandRateNormalizedS = 0.224;
	constexpr double kRollCommandRateNormalizedS = 0.256;
	constexpr double kPitchCenteringRetention = 0.98;
	constexpr double kRollYawCenteringRetention = 0.9;
	constexpr double kPitchPositiveCenteringThreshold = 0.7;
	constexpr double kPitchNegativeCenteringThreshold = -0.5;
	constexpr PrimaryAxisDynamics kPitchDynamics = {
		kPitchYawCommandRateNormalizedS,
		kPitchCenteringRetention,
		kPitchPositiveCenteringThreshold,
		kPitchNegativeCenteringThreshold,
		true
	};
	constexpr PrimaryAxisDynamics kRollDynamics = {
		kRollCommandRateNormalizedS,
		kRollYawCenteringRetention,
		0.0,
		0.0,
		false
	};
	constexpr PrimaryAxisDynamics kYawDynamics = {
		kPitchYawCommandRateNormalizedS,
		kRollYawCenteringRetention,
		0.0,
		0.0,
		false
	};
	PrimaryControlState next = current;
	next.pitch = update_primary_axis(current.pitch, kPitchDynamics, dt_s);
	next.roll = update_primary_axis(current.roll, kRollDynamics, dt_s);
	next.yaw = update_primary_axis(current.yaw, kYawDynamics, dt_s);
	next.pitch.trim = clamp_pitch_roll_trim(next.pitch.trim);
	next.roll.trim = clamp_pitch_roll_trim(next.roll.trim);
	next.yaw.trim = clamp_yaw_trim(next.yaw.trim);
	return next;
}

inline double normalize_throttle_axis(double raw_value, bool throttle_axis_inverted)
{
	double normalized = Common::limit((raw_value + 1.0) * 0.5, 0.0, 1.0);
	if (throttle_axis_inverted)
	{
		normalized = 1.0 - normalized;
	}
	return Common::limit(normalized, 0.0, 1.0);
}

inline bool throttle_axis_changed(double new_value, double old_value, double epsilon)
{
	const double delta = (new_value > old_value) ? (new_value - old_value) : (old_value - new_value);
	return delta > epsilon;
}

inline void set_throttle_axis_channel(ThrottleChannelState& channel, double normalized_value)
{
	if (throttle_axis_changed(normalized_value, channel.axis_cmd, 1e-4))
	{
		channel.use_axis = true;
	}
	channel.axis_cmd = normalized_value;
}

inline void set_common_throttle_axis(ThrottleInputState& throttles, double raw_value)
{
	const double normalized = normalize_throttle_axis(raw_value, throttles.axis_inverted);
	set_throttle_axis_channel(throttles.left, normalized);
	set_throttle_axis_channel(throttles.right, normalized);
}

inline void set_left_throttle_axis(ThrottleInputState& throttles, double raw_value)
{
	const double normalized = normalize_throttle_axis(raw_value, throttles.axis_inverted);
	set_throttle_axis_channel(throttles.left, normalized);
}

inline void set_right_throttle_axis(ThrottleInputState& throttles, double raw_value)
{
	const double normalized = normalize_throttle_axis(raw_value, throttles.axis_inverted);
	set_throttle_axis_channel(throttles.right, normalized);
}

inline double resolve_pilot_throttle_cmd(double axis_cmd, double keyboard_cmd, bool use_axis)
{
	return Common::limit(use_axis ? axis_cmd : keyboard_cmd, 0.0, 1.0);
}

inline double resolve_keyboard_throttle_base(double axis_cmd, double keyboard_cmd, bool use_axis)
{
	return Common::limit(use_axis ? axis_cmd : keyboard_cmd, 0.0, 1.0);
}

inline void step_keyboard_throttle_channel(ThrottleChannelState& channel, double delta)
{
	channel.keyboard_cmd = Common::limit(
		resolve_keyboard_throttle_base(channel.axis_cmd, channel.keyboard_cmd, channel.use_axis) + delta,
		0.0,
		1.0);
	channel.use_axis = false;
}

inline void step_common_keyboard_throttle(ThrottleInputState& throttles, double delta)
{
	step_keyboard_throttle_channel(throttles.left, delta);
	step_keyboard_throttle_channel(throttles.right, delta);
}

inline void step_left_keyboard_throttle(ThrottleInputState& throttles, double delta)
{
	step_keyboard_throttle_channel(throttles.left, delta);
}

inline void step_right_keyboard_throttle(ThrottleInputState& throttles, double delta)
{
	step_keyboard_throttle_channel(throttles.right, delta);
}

inline void update_pilot_throttle_cmds(ThrottleInputState& throttles)
{
	throttles.left.pilot_cmd = resolve_pilot_throttle_cmd(
		throttles.left.axis_cmd,
		throttles.left.keyboard_cmd,
		throttles.left.use_axis);
	throttles.right.pilot_cmd = resolve_pilot_throttle_cmd(
		throttles.right.axis_cmd,
		throttles.right.keyboard_cmd,
		throttles.right.use_axis);
}

}
