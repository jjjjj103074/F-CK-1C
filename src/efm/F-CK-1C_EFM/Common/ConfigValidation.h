#pragma once

#include <cmath>
#include <initializer_list>
#include <iterator>

namespace Common
{
inline bool all_finite(std::initializer_list<double> values)
{
	for (const double value : values)
	{
		if (!std::isfinite(value))
		{
			return false;
		}
	}
	return true;
}

template <typename Range>
bool all_finite(const Range& values)
{
	for (const double value : values)
	{
		if (!std::isfinite(value))
		{
			return false;
		}
	}
	return true;
}

template <typename Range>
bool finite_strictly_increasing(const Range& values)
{
	if (values.begin() == values.end())
	{
		return false;
	}
	auto previous = values.begin();
	if (!std::isfinite(*previous))
	{
		return false;
	}
	for (auto current = std::next(previous); current != values.end(); ++current)
	{
		if (!std::isfinite(*current) || *current <= *previous)
		{
			return false;
		}
		previous = current;
	}
	return true;
}
}
