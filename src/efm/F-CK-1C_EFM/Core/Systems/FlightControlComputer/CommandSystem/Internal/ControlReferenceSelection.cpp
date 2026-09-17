#include "ControlReferenceSelection.h"

// Command-system implementation detail; use FlightControlCommandSystem.

namespace Core
{
namespace Systems
{
SelectedFlightReference select_flight_reference(
	const CoordinatedManeuverReference& manual,
	const AutomaticFlightGuidanceReference& automatic)
{
	SelectedFlightReference selected;
	selected.longitudinal_authority = automatic.longitudinal_authority;
	selected.lateral_authority = automatic.lateral_authority;
	selected.directional_authority = manual.directional_authority;
	selected.longitudinal = automatic.longitudinal_authority ==
		AuthorityState::Automatic
		? SelectedLongitudinalFlightReference{
			AutomaticLongitudinalFlightReference{
				automatic.vertical_type,
				automatic.pitch_attitude_reference_rad,
				automatic.vertical_speed_reference_ft_s } }
		: SelectedLongitudinalFlightReference{ manual.longitudinal };
	selected.lateral = automatic.lateral_authority == AuthorityState::Automatic
		? SelectedLateralFlightReference{
			AutomaticLateralFlightReference{
				automatic.bank_angle_reference_rad } }
		: SelectedLateralFlightReference{
			ManualLateralFlightReference{
				manual.lateral_directional.roll_rate_reference_rad_s } };
	selected.directional = {
		manual.lateral_directional.sideslip_reference_rad,
		manual.lateral_directional.yaw_rate_feedforward_rad_s
	};
	return selected;
}
}
}
