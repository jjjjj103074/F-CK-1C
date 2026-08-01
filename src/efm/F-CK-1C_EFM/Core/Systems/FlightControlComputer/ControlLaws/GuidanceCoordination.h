#pragma once

#include "ConfigurationAndMode.h"
#include "ControlLawSignals.h"
#include "../FlightControlReferences.h"

namespace Core
{
namespace Systems
{
struct GuidanceCoordinationInput
{
	::Systems::ConditionedFlightControlInput flight;
	SelectedFlightReference selected;
	::Systems::ManeuverEnvelope envelope;
	::Systems::FBWControllerConfig config;
};

GuidanceCoordinationResult coordinate_guidance(
	const GuidanceCoordinationInput& input);
}
}
