#pragma once

#include "Units.h"

#include <cmath>

namespace Common
{
inline double wrap_heading_rad(double angle_rad)
{
	const double period_rad = 2.0 * kPi;
	double wrapped_rad = std::fmod(angle_rad, period_rad);
	if (wrapped_rad < 0.0) wrapped_rad += period_rad;
	return wrapped_rad;
}

inline double wrap_signed_angle_rad(double angle_rad)
{
	return std::atan2(std::sin(angle_rad), std::cos(angle_rad));
}

inline double shortest_angle_difference_rad(
	double target_rad,
	double current_rad)
{
	return wrap_signed_angle_rad(target_rad - current_rad);
}
}
