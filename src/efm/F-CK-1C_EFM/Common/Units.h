#pragma once

namespace Common
{
static constexpr double kPi = 3.1415926535897932384626433832795;
static constexpr double kDegPerRad = 57.295779513082320876798154814105;

inline double rad(double degrees)
{
	return degrees / kDegPerRad;
}

inline double deg(double radians)
{
	return radians * kDegPerRad;
}
}
