#pragma once

#include "../FlightControlReferences.h"

namespace Core
{
namespace Systems
{
SelectedFlightReference select_flight_reference(
	const CoordinatedManeuverReference& manual,
	const AutomaticFlightGuidanceReference& automatic);
}
}
