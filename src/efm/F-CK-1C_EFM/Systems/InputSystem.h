#pragma once

#include "../Common/Clamp.h"

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
	set_primary_axis_input(controls.yaw, -value);
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

inline double update_pitch_axis_input(double input, int discrete, bool analog)
{
	if (analog == true)
	{
		return Common::limit(input, -1.0, 1.0);
	}

	if (discrete > 0.1)
	{
		input += 0.0035;
		if (input > 1.0)
		{
			input = 1.0;
		}
	}
	if (discrete == 0 && input > 0.5)
	{
		if (input > 0.7)
		{
			input *= 0.98;
		}
	}
	if (discrete < -0.1)
	{
		input -= 0.0035;
		if (input < -1.0)
		{
			input = -1.0;
		}
	}
	if (discrete == 0 && input < -0.5)
	{
		if (input < -0.5)
		{
			input *= 0.98;
		}
	}

	return input;
}

inline double update_roll_axis_input(double input, int discrete, bool analog)
{
	if (analog == true)
	{
		return Common::limit(input, -1.0, 1.0);
	}

	if (discrete > 0.1)
	{
		input += 0.004;
		if (input > 1.0)
		{
			input = 1.0;
		}
	}
	if (discrete < -0.1)
	{
		input -= 0.004;
		if (input < -1.0)
		{
			input = -1.0;
		}
	}
	if (discrete == 0)
	{
		input *= 0.9;
	}

	return input;
}

inline double update_yaw_axis_input(double input, int discrete, bool analog)
{
	if (analog == true)
	{
		return Common::limit(input, -1.0, 1.0);
	}

	if (discrete > 0.1)
	{
		input += 0.0035;
		if (input > 1.0)
		{
			input = 1.0;
		}
	}
	if (discrete < -0.1)
	{
		input -= 0.0035;
		if (input < -1.0)
		{
			input = -1.0;
		}
	}
	if (discrete == 0)
	{
		input *= 0.9;
	}

	return input;
}

inline double clamp_pitch_roll_trim(double trim)
{
	return Common::limit(trim, -0.3, 0.3);
}

inline double clamp_yaw_trim(double trim)
{
	return Common::limit(trim, -0.2, 0.2);
}

inline void update_primary_control_inputs(PrimaryControlState& controls)
{
	controls.pitch.input = update_pitch_axis_input(controls.pitch.input, controls.pitch.discrete, controls.pitch.analog);
	controls.pitch.trim = clamp_pitch_roll_trim(controls.pitch.trim);

	controls.roll.input = update_roll_axis_input(controls.roll.input, controls.roll.discrete, controls.roll.analog);
	controls.roll.trim = clamp_pitch_roll_trim(controls.roll.trim);

	controls.yaw.input = update_yaw_axis_input(controls.yaw.input, controls.yaw.discrete, controls.yaw.analog);
	controls.yaw.trim = clamp_yaw_trim(controls.yaw.trim);
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

inline double compose_engine_throttle_cmd(
	double pilot_cmd,
	double fbw_cmd,
	bool fbw_throttle_override,
	double fbw_throttle_blend)
{
	const double pilot = Common::limit(pilot_cmd, 0.0, 1.0);
	const double fbw = Common::limit(fbw_cmd, 0.0, 1.0);

	if (fbw_throttle_override)
	{
		return fbw;
	}

	const double blend = Common::limit(fbw_throttle_blend, 0.0, 1.0);
	return Common::limit((1.0 - blend) * pilot + blend * fbw, 0.0, 1.0);
}
}
