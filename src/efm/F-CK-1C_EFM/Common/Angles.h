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

inline int wrap_heading_deg(int angle_deg)
{
	int wrapped_deg = angle_deg % 360;
	if (wrapped_deg < 0) wrapped_deg += 360;
	return wrapped_deg;
}

inline double wrap_heading_deg(double angle_deg)
{
	double wrapped_deg = std::fmod(angle_deg, 360.0);
	if (wrapped_deg < 0.0) wrapped_deg += 360.0;
	return wrapped_deg;
}

inline double shortest_heading_difference_deg(
	double target_deg,
	double current_deg)
{
	double difference_deg = wrap_heading_deg(target_deg - current_deg);
	if (difference_deg >= 180.0) difference_deg -= 360.0;
	return difference_deg;
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
