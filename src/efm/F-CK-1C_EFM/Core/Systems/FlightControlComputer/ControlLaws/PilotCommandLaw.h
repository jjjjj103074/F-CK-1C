#pragma once

#include "ConfigurationAndMode.h"
#include "ControlLawSignals.h"
#include "../FlightControlReferences.h"

namespace Systems
{
struct PilotCommandLawInput
{
	ConditionedFlightControlInput flight;
	FBWControllerConfig config;
	FBWCatParams cat;
	FBWGainScheduleValues gains;
	ManeuverEnvelope envelope;
};

Core::Systems::CoordinatedManeuverReference make_pilot_maneuver_reference(
	const PilotCommandLawInput& input);
}
