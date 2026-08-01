#pragma once

#include "FlightControlStatus.h"

namespace Core
{
namespace Systems
{
enum class VerticalGuidanceReferenceType
{
	None,
	PitchAttitude,
	VerticalSpeed
};

struct AutomaticFlightGuidanceReference
{
	AuthorityState longitudinal_authority = AuthorityState::Manual;
	AuthorityState lateral_authority = AuthorityState::Manual;
	VerticalGuidanceReferenceType vertical_type =
		VerticalGuidanceReferenceType::None;
	double pitch_attitude_reference_rad = 0.0;
	double vertical_speed_reference_mps = 0.0;
	double bank_angle_reference_rad = 0.0;
	double sideslip_reference_rad = 0.0;
	bool experimental_auto_throttle_engaged = false;
	double experimental_throttle_normalized = 0.0;
};

struct LongitudinalManeuverReference
{
	double normal_acceleration_reference_g = 1.0;
	double pitch_rate_feedforward_rad_s = 0.0;
};

struct LateralDirectionalManeuverReference
{
	double roll_rate_reference_rad_s = 0.0;
	double sideslip_reference_rad = 0.0;
	double yaw_rate_feedforward_rad_s = 0.0;
};

struct CoordinatedManeuverReference
{
	LongitudinalManeuverReference longitudinal;
	LateralDirectionalManeuverReference lateral_directional;
	AuthorityState longitudinal_authority = AuthorityState::Manual;
	AuthorityState lateral_authority = AuthorityState::Manual;
	AuthorityState directional_authority = AuthorityState::Manual;
};

struct SelectedFlightReference
{
	AutomaticFlightGuidanceReference automatic;
	CoordinatedManeuverReference manual;
	AuthorityState longitudinal_authority = AuthorityState::Manual;
	AuthorityState lateral_authority = AuthorityState::Manual;
	AuthorityState directional_authority = AuthorityState::Manual;
};

struct GuidanceConstraint
{
	ConstraintReason reason = ConstraintReason::None;
	bool vertical_constrained = false;
	bool lateral_constrained = false;
};

struct GuidanceCoordinationResult
{
	CoordinatedManeuverReference reference;
	GuidanceConstraint constraint;
};
}
}
