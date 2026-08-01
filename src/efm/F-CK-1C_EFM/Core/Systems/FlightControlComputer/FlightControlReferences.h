#pragma once

#include "FlightControlStatus.h"

#include <variant>

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

struct AutomaticLongitudinalFlightReference
{
	VerticalGuidanceReferenceType type =
		VerticalGuidanceReferenceType::None;
	double pitch_attitude_reference_rad = 0.0;
	double vertical_speed_reference_mps = 0.0;
};

struct ManualLateralFlightReference
{
	double roll_rate_reference_rad_s = 0.0;
};

struct AutomaticLateralFlightReference
{
	double bank_angle_reference_rad = 0.0;
};

struct SelectedDirectionalFlightReference
{
	double sideslip_reference_rad = 0.0;
	double yaw_rate_feedforward_rad_s = 0.0;
};

using SelectedLongitudinalFlightReference = std::variant<
	LongitudinalManeuverReference,
	AutomaticLongitudinalFlightReference>;
using SelectedLateralFlightReference = std::variant<
	ManualLateralFlightReference,
	AutomaticLateralFlightReference>;

struct SelectedFlightReference
{
	SelectedLongitudinalFlightReference longitudinal;
	SelectedLateralFlightReference lateral;
	SelectedDirectionalFlightReference directional;
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
