#pragma once

#include "../../ModeAndGainScheduling/ModeAndGainScheduling.h"
#include "../../ControlLaws/ControlLawSignals.h"
#include "../../Contracts/FlightControlReferences.h"

// Private pilot-command mapping policy for FlightControlCommandSystem.

namespace Systems
{
struct PilotCommandLawInput
{
	ManagedFlightControlSignals signals;
	LongitudinalControlConfig longitudinal;
	StoresControlLawSchedule stores;
	GainScheduleValues gains;
	ManeuverEnvelope envelope;
};

Core::Systems::CoordinatedManeuverReference make_pilot_maneuver_reference(
	const PilotCommandLawInput& input);
}
