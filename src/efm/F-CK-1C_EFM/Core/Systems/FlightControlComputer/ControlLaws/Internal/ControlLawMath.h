#pragma once

#include "../ControlLawConfig.h"

// Private numerical helpers for the control-law module.
#include "Common/Clamp.h"
#include "Common/Units.h"
#include <cmath>

namespace Systems
{
inline double fbw_blend_value(double a, double b, double t)
{
	return a + (b - a) * Common::limit(t, 0.0, 1.0);
}

inline double fbw_min(double a, double b)
{
	return (a < b) ? a : b;
}

inline double fbw_max(double a, double b)
{
	return (a > b) ? a : b;
}

inline double fbw_wrap_pi(double angle)
{
	double wrapped = angle;
	while (wrapped > Common::kPi)
	{
		wrapped -= 2.0 * Common::kPi;
	}
	while (wrapped < -Common::kPi)
	{
		wrapped += 2.0 * Common::kPi;
	}
	return wrapped;
}

inline double fbw_soft_limit_symmetric(double command, double limit_value)
{
	if (limit_value <= 1e-6)
	{
		return 0.0;
	}
	return limit_value * std::tanh(command / limit_value);
}

inline double fbw_smoothstep01(double value)
{
	const double limited = Common::limit(value, 0.0, 1.0);
	return limited * limited * (3.0 - 2.0 * limited);
}

inline double fbw_soft_clip_positive(double value, double soft_limit, double hard_limit)
{
	if (value <= soft_limit)
	{
		return value;
	}
	if (value >= hard_limit)
	{
		return hard_limit;
	}
	const double ratio = (value - soft_limit) / (hard_limit - soft_limit);
	return soft_limit + (hard_limit - soft_limit) * fbw_smoothstep01(ratio);
}

inline double fbw_soft_clip_negative(double value, double soft_limit, double hard_limit)
{
	if (value >= soft_limit)
	{
		return value;
	}
	if (value <= hard_limit)
	{
		return hard_limit;
	}
	const double ratio = (soft_limit - value) / (soft_limit - hard_limit);
	return soft_limit - (soft_limit - hard_limit) * fbw_smoothstep01(ratio);
}

}
