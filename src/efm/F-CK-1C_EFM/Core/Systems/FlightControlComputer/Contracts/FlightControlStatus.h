#pragma once

namespace Core
{
namespace Systems
{
// Operational FLCC state belongs to the flight-control domain. Cockpit
// snapshot enums are a separate projection mapped by diagnostics.
enum class AutomaticFlightControlVerticalMode
{
	Off,
	PitchAttitudeHold,
	AltitudeHold
};

enum class AutomaticFlightControlLateralMode
{
	Off,
	RollAttitudeHold,
	HeadingSelect
};

enum class AutomaticFlightControlReason
{
	None,
	Commanded,
	BelowMinimumIndicatedAirspeed,
	WeightOnWheels,
	RollLimit,
	PitchLimit,
	MachLimit,
	MagneticHeadingUnavailable,
	PressureAltitudeUnavailable
};

enum class AuthorityState
{
	Manual,
	Automatic,
	Bypassed,
	StickSteering
};

enum class ConstraintReason
{
	None,
	GuidanceBankLimit,
	GuidanceLoadFactorLimit,
	LateralConstrainedByVerticalAuthority,
	VerticalReferenceUnmaintainable,
	HardAngleOfAttackLimit,
	HardLoadFactorLimit,
	HardRateLimit,
	ActuatorAuthority
};

enum class DegradationReason
{
	None,
	SustainedVerticalTrackingFailure,
	SustainedLateralTrackingFailure,
	SustainedActuatorSaturation,
	MagneticHeadingUnavailable,
	PressureAltitudeUnavailable
};

enum class DisconnectReason
{
	None,
	PilotCommand,
	SafetyCondition,
	InvalidInput
};
}
}
