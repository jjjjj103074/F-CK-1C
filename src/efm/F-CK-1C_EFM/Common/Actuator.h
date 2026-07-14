#pragma once

namespace Common
{
struct ActuatorInput
{
	double target = 0.0;
	double down_speed = 0.0;
	double up_speed = 0.0;
};

inline double actuator(double value, const ActuatorInput& input)
{
	if ((value + input.up_speed) < input.target)
	{
		return value + input.up_speed;
	}
	if ((value + input.down_speed) > input.target)
	{
		return value + input.down_speed;
	}
	return input.target;
}
}
