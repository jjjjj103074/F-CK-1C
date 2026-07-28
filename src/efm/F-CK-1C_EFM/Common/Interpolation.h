#pragma once

#include <cmath>
#include <stdexcept>

namespace Common
{
inline double rescale(double input, double min, double max)
{
	if (!std::isfinite(input))
	{
		throw std::domain_error("Common::rescale input is not finite.");
	}
	if (!std::isfinite(min))
	{
		throw std::domain_error("Common::rescale minimum is not finite.");
	}
	if (!std::isfinite(max))
	{
		throw std::domain_error("Common::rescale maximum is not finite.");
	}
	return input >= 0.0
		? input * std::fabs(max)
		: input * std::fabs(min);
}

inline double smooth_lerp(double current, double target, double t)
{
	return current + (target - current) * t;
}
}
