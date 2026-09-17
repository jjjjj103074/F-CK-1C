#pragma once

#include "../ControlLawConfig.h"
#include "../ControlLawSignals.h"
#include "../ControlLawState.h"

// Private stateful longitudinal seam of FlightControlLaws.

namespace Systems
{
struct LongitudinalAxisResult
{
	double effort_normalized = 0.0;
	FlightControlLawsStatus status;
	FlightControlLawsDiagnostics diagnostics;
};

LongitudinalAxisResult update_longitudinal_control_law(
	LongitudinalControlLawState& state,
	const FlightControlLawsConfig& config,
	const FlightControlLawsInput& input);
}
