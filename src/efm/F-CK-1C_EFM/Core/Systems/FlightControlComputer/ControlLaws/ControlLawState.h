#pragma once

#include "../Contracts/FlightControlReferences.h"

namespace Systems
{
struct LongitudinalControlLawState
{
	double normal_acceleration_integral_effort = 0.0;
	double pitch_rate_integral_effort = 0.0;
	double pitch_rate_low_pass_rad_s = 0.0;
	Core::Systems::LongitudinalCommandMode active_mode =
		Core::Systems::LongitudinalCommandMode::NormalAcceleration;
	bool initialized = false;
};

struct LateralControlLawState
{
	double roll_integral = 0.0;
};

struct DirectionalControlLawState
{
	double yaw_integral = 0.0;
};
}
