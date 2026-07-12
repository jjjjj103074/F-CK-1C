#pragma once
#include "stdafx.h"
#include "Common/Actuator.h"
#include "Common/Clamp.h"
#include "Common/Interpolation.h"
#include "Common/PathUtils.h"
#include "Common/Table.h"
#include "Common/Units.h"
#include "Common/Vec3.h"

// Compatibility facade for legacy EFM code.
// New code should include Common/* directly and use the Common namespace.

// EFMREF: COMMON_UTIL - Unit conversion compatibility wrapper.
inline double rad(double x)
{
	return Common::rad(x);
}

// EFMREF: COMMON_UTIL - Unit conversion compatibility wrapper.
inline double deg(double x)
{
	return Common::deg(x);
}

// EFMREF: COMMON_UTIL - Generic rate-limited actuator compatibility wrapper.
inline double actuator(double value, double target, double down_speed, double up_speed)
{
	return Common::actuator(value, target, down_speed, up_speed);
}

// EFMREF: COMMON_UTIL - Generic clamp compatibility wrapper.
inline double limit(double input, double lower_limit, double upper_limit)
{
	return Common::limit(input, lower_limit, upper_limit);
}

// EFMREF: COMMON_UTIL - Axis rescale compatibility wrapper.
inline double rescale(double input, double min, double max)
{
	return Common::rescale(input, min, max);
}

using Vec3 = Common::Vec3;

// EFMREF: COMMON_UTIL - Vec3 cross product compatibility alias.
using Common::cross;

// EFMREF: COMMON_UTIL - Table interpolation compatibility wrapper.
inline double lerp(double* x, double* f, unsigned sz, double t)
{
	return Common::lerp(x, f, sz, t);
}

// EFMREF: COMMON_UTIL - Scalar smoothing compatibility wrapper.
inline double smooth_lerp(double current, double target, double t)
{
	return Common::smooth_lerp(current, target, t);
}
