#pragma once

#include "../Common/Actuator.h"
#include "../Common/Clamp.h"
#include "../Common/Units.h"
#include <array>
#include <cmath>

namespace Systems
{
static const unsigned kLandingGearWheelCount = 3;

struct WheelState
{
	double brake = 0.0;
	double brake_left = 0.0;
	double brake_right = 0.0;
	double spin[kLandingGearWheelCount] = { 0.0, 0.0, 0.0 };
	double nose_steering = 0.0;
	bool nose_turn_enabled = true;
};

struct LandingGearSystemState
{
	bool switch_down = false;
	double position = 0.0;
	WheelState wheels;
};

struct WheelSpinInput
{
	double ground_speed = 0.0;
	double dt = 0.0;
	double altitude_agl = 0.0;
	std::array<double, kLandingGearWheelCount> wheel_radius = {};
};

inline void toggle_gear(LandingGearSystemState& landing_gear)
{
	landing_gear.switch_down = !landing_gear.switch_down;
}

inline void set_gear(LandingGearSystemState& landing_gear, bool down)
{
	landing_gear.switch_down = down;
}

inline void update_gear_position(LandingGearSystemState& landing_gear)
{
	landing_gear.position = Common::limit(
		Common::actuator(
			landing_gear.position,
			{ landing_gear.switch_down ? 1.0 : 0.0, -0.001, 0.001 }),
		0.0,
		1.0);
}

inline void reset_wheel_brakes(WheelState& wheels)
{
	wheels.brake = 0.0;
	wheels.brake_left = 0.0;
	wheels.brake_right = 0.0;
}

inline void reset_wheel_spin(WheelState& wheels)
{
	for (unsigned i = 0; i < kLandingGearWheelCount; ++i)
	{
		wheels.spin[i] = 0.0;
	}
	wheels.nose_steering = 0.0;
}

inline double compute_nose_wheel_steering(
	const LandingGearSystemState& landing_gear,
	double v_scalar,
	double yaw_input)
{
	if (!landing_gear.wheels.nose_turn_enabled ||
		landing_gear.position <= 0.5 ||
		v_scalar >= 70.0)
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
		Common::actuator(wheels.nose_steering, { target_steering, -0.06, 0.06 }),
		-1.0,
		1.0);
}

inline void update_wheel_spin(
	WheelState& wheels,
	double gear_position,
	const WheelSpinInput& input)
{
	const double spin_enable = ((gear_position > 0.2) && (input.altitude_agl < 2.5)) ? 1.0 : 0.0;
	for (unsigned i = 0; i < kLandingGearWheelCount; ++i)
	{
		const double wheel_circumference = 2.0 * Common::kPi * input.wheel_radius[i];
		if (wheel_circumference <= 1e-6)
		{
			continue;
		}
		wheels.spin[i] = std::fmod(
			wheels.spin[i] +
				(input.ground_speed / wheel_circumference) * input.dt * spin_enable,
			1.0);
		if (wheels.spin[i] < 0.0)
		{
			wheels.spin[i] += 1.0;
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

inline double normalize_brake_axis(double raw_value)
{
	if (raw_value < 0.0)
	{
		return Common::limit((raw_value + 1.0) * 0.5, 0.0, 1.0);
	}
	return Common::limit(raw_value, 0.0, 1.0);
}

inline void reset_wheels(WheelState& wheels)
{
	reset_wheel_brakes(wheels);
	reset_wheel_spin(wheels);
}

inline void configure_ground_start_landing_gear(LandingGearSystemState& landing_gear)
{
	reset_wheels(landing_gear.wheels);
	landing_gear.switch_down = true;
	landing_gear.position = 1.0;
	landing_gear.wheels.nose_turn_enabled = true;
}

inline void configure_air_start_landing_gear(LandingGearSystemState& landing_gear)
{
	reset_wheels(landing_gear.wheels);
	landing_gear.switch_down = false;
	landing_gear.position = 0.0;
	landing_gear.wheels.nose_turn_enabled = false;
}

inline void configure_release_landing_gear(LandingGearSystemState& landing_gear)
{
	landing_gear.wheels.nose_turn_enabled = false;
}
}
