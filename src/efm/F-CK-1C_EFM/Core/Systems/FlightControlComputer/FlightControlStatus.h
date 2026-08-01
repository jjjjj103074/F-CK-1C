#pragma once

namespace Core
{
namespace Systems
{
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
	SustainedActuatorSaturation
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
