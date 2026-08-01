#include "ControlReferenceSelection.h"

namespace Core
{
namespace Systems
{
SelectedFlightReference select_flight_reference(
	const CoordinatedManeuverReference& manual,
	const AutomaticFlightGuidanceReference& automatic)
{
	return {
		automatic,
		manual,
		automatic.longitudinal_authority,
		automatic.lateral_authority,
		manual.directional_authority
	};
}
}
}
