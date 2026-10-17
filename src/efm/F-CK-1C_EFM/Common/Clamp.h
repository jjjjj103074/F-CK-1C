#pragma once

namespace Common
{
inline double limit(double input, double lower_limit, double upper_limit)
{
	if (input > upper_limit)
	{
		return upper_limit;
	}
	if (input < lower_limit)
	{
		return lower_limit;
	}
	return input;
}
}
