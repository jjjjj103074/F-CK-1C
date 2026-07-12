#pragma once

namespace Common
{
inline double actuator(double value, double target, double down_speed, double up_speed)
{
	if ((value + up_speed) < target)
	{
		return value + up_speed;
	}
	if ((value + down_speed) > target)
	{
		return value + down_speed;
	}
	return target;
}
}
