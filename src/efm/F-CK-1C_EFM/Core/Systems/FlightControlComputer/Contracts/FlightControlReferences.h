#pragma once

#include "FlightControlStatus.h"

#include <variant>

namespace Core
{
namespace Systems
{
// Aircraft-system guidance uses feet/second for vertical speed and radians for
// attitude/body-rate objectives. Pilot heading selection remains whole
// magnetic degrees until LateralGuidance emits a bank reference in radians.
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
	double vertical_speed_reference_ft_s = 0.0;
	double bank_angle_reference_rad = 0.0;
	bool experimental_auto_throttle_engaged = false;
	double experimental_throttle_normalized = 0.0;
};

enum class LongitudinalCommandMode
{
	NormalAcceleration,
	PitchRate
};

struct NormalAccelerationCommand
{
	double target_g = 1.0;
};

struct PitchRateCommand
{
	double target_rad_s = 0.0;
};

using LongitudinalCommand = std::variant<
	NormalAccelerationCommand,
	PitchRateCommand>;

struct LongitudinalManeuverReference
{
	LongitudinalCommand command = NormalAccelerationCommand{};
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
	double vertical_speed_reference_ft_s = 0.0;
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
