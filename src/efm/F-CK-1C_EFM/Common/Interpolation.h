#pragma once

#include <math.h>

namespace Common
{
inline double rescale(double input, double min, double max)
{
	if (input >= 0.0)
	{
		return input * fabs(max);
	}
	if (input < 0.0)
	{
		return input * fabs(min);
	}
}

inline double smooth_lerp(double current, double target, double t)
{
	return current + (target - current) * t;
}
}
