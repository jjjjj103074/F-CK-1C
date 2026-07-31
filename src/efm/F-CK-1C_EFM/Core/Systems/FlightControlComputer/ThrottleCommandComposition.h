#pragma once

#include "Common/Clamp.h"

namespace Systems
{
struct ThrottleCompositionInput
{
	double pilot_command = 0.0;
	double automatic_command = 0.0;
	double automatic_blend = 0.0;
	bool automatic_override = false;
};

inline double compose_engine_throttle_command(
	const ThrottleCompositionInput& input)
{
	const double pilot = Common::limit(input.pilot_command, 0.0, 1.0);
	const double automatic = Common::limit(
		input.automatic_command, 0.0, 1.0);
	if (input.automatic_override)
	{
		return automatic;
	}
	const double blend = Common::limit(
		input.automatic_blend, 0.0, 1.0);
	return Common::limit(
		(1.0 - blend) * pilot + blend * automatic,
		0.0,
		1.0);
}
}
