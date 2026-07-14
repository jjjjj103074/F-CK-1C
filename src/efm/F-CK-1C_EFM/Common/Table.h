#pragma once

namespace Common
{
struct LinearTable
{
	const double* x = nullptr;
	const double* values = nullptr;
	unsigned size = 0;
};

inline double interpolate_segment(
	const LinearTable& table,
	unsigned index,
	double input)
{
	const double denominator = table.x[index] - table.x[index - 1];
	return ((table.values[index] - table.values[index - 1]) / denominator * input) +
		(table.x[index] * table.values[index - 1] -
			table.x[index - 1] * table.values[index]) / denominator;
}

inline double lerp(const LinearTable& table, double input)
{
	for (unsigned index = 0; index < table.size; ++index)
	{
		if (!(input <= table.x[index]))
		{
			continue;
		}
		if (index == 0)
		{
			return table.values[0];
		}
		return interpolate_segment(table, index, input);
	}
	return table.values[table.size - 1];
}
}
