#pragma once

#include "ControlLawSignals.h"
#include "ControlLawState.h"

namespace Systems
{
void reset_fbw_state(
	FBWControllerState& state,
	const FlightControlLawResetInput& input);

void reset_fbw_throttle_interface(FBWControllerState& state);
}
