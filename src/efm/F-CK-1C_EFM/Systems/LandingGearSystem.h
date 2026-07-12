#pragma once

#include "../Common/Actuator.h"
#include "../Common/Clamp.h"
#include <cmath>

namespace Systems
{
struct WheelState
{
	double brake = 0.0;
	double brake_left = 0.0;
	double brake_right = 0.0;
	double spin[3] = { 0.0, 0.0, 0.0 };
	double nose_steering = 0.0;
	bool nose_turn_enabled = true;
};

inline void reset_wheel_brakes(WheelState& wheels)
{
	wheels.brake = 0.0;
	wheels.brake_left = 0.0;
	wheels.brake_right = 0.0;
}

inline void reset_wheel_spin(WheelState& wheels)
{
	for (int i = 0; i < 3; ++i)
	{
		wheels.spin[i] = 0.0;
	}
	wheels.nose_steering = 0.0;
}

inline double compute_nose_wheel_steering(
	const WheelState& wheels,
	double gear_pos,
	double v_scalar,
	double yaw_input)
{
	if (!wheels.nose_turn_enabled || gear_pos <= 0.5 || v_scalar >= 70.0)
	{
		return 0.0;
	}

	return -Common::limit(yaw_input, -1.0, 1.0) * 0.75;
}

inline void update_nose_wheel_steering(
	WheelState& wheels,
	double target_steering)
{
	wheels.nose_steering = Common::limit(
		Common::actuator(wheels.nose_steering, target_steering, -0.06, 0.06),
		-1.0,
		1.0);
}

inline void update_wheel_spin(
	WheelState& wheels,
	double ground_speed,
	double dt,
	double gear_pos,
	double altitude_agl,
	const double wheel_radius[3],
	double pi)
{
	const double spin_enable = ((gear_pos > 0.2) && (altitude_agl < 2.5)) ? 1.0 : 0.0;
	for (int i = 0; i < 3; ++i)
	{
		const double wheel_circumference = 2.0 * pi * wheel_radius[i];
		if (wheel_circumference > 1e-6)
		{
			wheels.spin[i] = std::fmod(wheels.spin[i] + (ground_speed / wheel_circumference) * dt * spin_enable, 1.0);
			if (wheels.spin[i] < 0.0)
			{
				wheels.spin[i] += 1.0;
			}
		}
	}
}

inline void set_brake_axis(WheelState& wheels, double value)
{
	wheels.brake = value;
	wheels.brake_left = wheels.brake;
	wheels.brake_right = wheels.brake;
}

inline void set_left_brake(WheelState& wheels, double value)
{
	wheels.brake_left = value;
	wheels.brake = (wheels.brake_left + wheels.brake_right) * 0.5;
}

inline void set_right_brake(WheelState& wheels, double value)
{
	wheels.brake_right = value;
	wheels.brake = (wheels.brake_left + wheels.brake_right) * 0.5;
}

inline void set_nose_turn_enabled(WheelState& wheels, bool enabled)
{
	wheels.nose_turn_enabled = enabled;
}

inline void toggle_nose_turn_enabled(WheelState& wheels, bool command_pressed)
{
	if (command_pressed)
	{
		wheels.nose_turn_enabled = !wheels.nose_turn_enabled;
	}
}
}
