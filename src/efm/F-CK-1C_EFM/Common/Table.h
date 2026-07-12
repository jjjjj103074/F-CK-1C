#pragma once

namespace Common
{
inline double lerp(double* x, double* f, unsigned sz, double t)
{
	for (unsigned i = 0; i < sz; i++)
	{
		if (t <= x[i])
		{
			if (i > 0)
			{
				return ((f[i] - f[i - 1]) / (x[i] - x[i - 1]) * t +
					(x[i] * f[i - 1] - x[i - 1] * f[i]) / (x[i] - x[i - 1]));
			}
			return f[0];
		}
	}
	return f[sz - 1];
}
}
