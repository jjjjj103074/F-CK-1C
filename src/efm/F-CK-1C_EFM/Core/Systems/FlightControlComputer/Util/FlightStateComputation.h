#pragma once

#include "../ControlLaws/ControlLawSignals.h"

namespace Core
{
namespace Systems
{
::Systems::ComputedFlightState compute_flight_state(
	const ::Systems::ManagedFlightControlSignals& signals);
}
}
