#pragma once

namespace Common
{
static constexpr double kPi = 3.1415926535897932384626433832795;
static constexpr double kDegPerRad = 57.295779513082320876798154814105;
static constexpr double kMetersPerSecondPerKnot = 0.5144444444444445;
static constexpr double kMetersPerFoot = 0.3048;

constexpr double rad(double angle_deg)
{
	return angle_deg / kDegPerRad;
}

constexpr double deg(double angle_rad)
{
	return angle_rad * kDegPerRad;
}

constexpr double feet(double metres)
{
	return metres / kMetersPerFoot;
}

constexpr double metres(double feet_value)
{
	return feet_value * kMetersPerFoot;
}
}
