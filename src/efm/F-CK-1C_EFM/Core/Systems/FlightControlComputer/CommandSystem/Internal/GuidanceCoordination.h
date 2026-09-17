#pragma once

#include "../../ModeAndGainScheduling/ModeAndGainScheduling.h"
#include "../../ControlLaws/ControlLawSignals.h"
#include "../../Contracts/FlightControlReferences.h"

// Private cross-axis coordination policy for FlightControlCommandSystem.

namespace Core
{
namespace Systems
{
struct GuidanceCoordinationInput
{
	::Systems::ComputedFlightState flight;
	SelectedFlightReference selected;
	::Systems::ManeuverEnvelope envelope;
	::Systems::GuidanceCoordinationConfig config;
	bool selected_gear_handle_down = false;
};

GuidanceCoordinationResult coordinate_guidance(
	const GuidanceCoordinationInput& input);
}
}
