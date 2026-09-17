#pragma once

#include "../../Contracts/FlightControlReferences.h"

// Private reference-selection policy for FlightControlCommandSystem.

namespace Core
{
namespace Systems
{
SelectedFlightReference select_flight_reference(
	const CoordinatedManeuverReference& manual,
	const AutomaticFlightGuidanceReference& automatic);
}
}
