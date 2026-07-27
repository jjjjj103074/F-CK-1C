#pragma once

#include "ControlLawTypes.h"

namespace Systems
{
void reset_fbw_state(
	FBWControllerState& state,
	const FBWResetInput& input);

void reset_fbw_throttle_interface(FBWControllerState& state);
}
