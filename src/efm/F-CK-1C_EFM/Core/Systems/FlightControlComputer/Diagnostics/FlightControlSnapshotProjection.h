#pragma once

#include "../Contracts/FlightControlStatus.h"
#include "../../../Contracts/CockpitContracts.h"

#include <stdexcept>

namespace Core
{
namespace Systems
{
namespace SnapshotProjection
{
inline Core::AutomaticFlightControlVerticalMode vertical_mode(
	AutomaticFlightControlVerticalMode value)
{
	switch (value)
	{
	case AutomaticFlightControlVerticalMode::Off:
		return Core::AutomaticFlightControlVerticalMode::Off;
	case AutomaticFlightControlVerticalMode::PitchAttitudeHold:
		return Core::AutomaticFlightControlVerticalMode::PitchAttitudeHold;
	case AutomaticFlightControlVerticalMode::AltitudeHold:
		return Core::AutomaticFlightControlVerticalMode::AltitudeHold;
	}
	throw std::logic_error("Unknown FLCC vertical mode.");
}

inline Core::AutomaticFlightControlLateralMode lateral_mode(
	AutomaticFlightControlLateralMode value)
{
	switch (value)
	{
	case AutomaticFlightControlLateralMode::Off:
		return Core::AutomaticFlightControlLateralMode::Off;
	case AutomaticFlightControlLateralMode::RollAttitudeHold:
		return Core::AutomaticFlightControlLateralMode::RollAttitudeHold;
	case AutomaticFlightControlLateralMode::HeadingSelect:
		return Core::AutomaticFlightControlLateralMode::HeadingSelect;
	}
	throw std::logic_error("Unknown FLCC lateral mode.");
}

inline Core::FlightControlAuthorityState authority(AuthorityState value)
{
	switch (value)
	{
	case AuthorityState::Manual:
		return Core::FlightControlAuthorityState::Manual;
	case AuthorityState::Automatic:
		return Core::FlightControlAuthorityState::Automatic;
	case AuthorityState::Bypassed:
		return Core::FlightControlAuthorityState::Bypassed;
	case AuthorityState::StickSteering:
		return Core::FlightControlAuthorityState::StickSteering;
	}
	throw std::logic_error("Unknown FLCC authority state.");
}

inline Core::FlightControlConstraintReason constraint(ConstraintReason value)
{
	switch (value)
	{
	case ConstraintReason::None:
		return Core::FlightControlConstraintReason::None;
	case ConstraintReason::GuidanceBankLimit:
		return Core::FlightControlConstraintReason::GuidanceBankLimit;
	case ConstraintReason::GuidanceLoadFactorLimit:
		return Core::FlightControlConstraintReason::GuidanceLoadFactorLimit;
	case ConstraintReason::LateralConstrainedByVerticalAuthority:
		return Core::FlightControlConstraintReason::
			LateralConstrainedByVerticalAuthority;
	case ConstraintReason::VerticalReferenceUnmaintainable:
		return Core::FlightControlConstraintReason::
			VerticalReferenceUnmaintainable;
	case ConstraintReason::HardAngleOfAttackLimit:
		return Core::FlightControlConstraintReason::HardAngleOfAttackLimit;
	case ConstraintReason::HardLoadFactorLimit:
		return Core::FlightControlConstraintReason::HardLoadFactorLimit;
	case ConstraintReason::HardRateLimit:
		return Core::FlightControlConstraintReason::HardRateLimit;
	case ConstraintReason::ActuatorAuthority:
		return Core::FlightControlConstraintReason::ActuatorAuthority;
	}
	throw std::logic_error("Unknown FLCC constraint reason.");
}

inline Core::FlightControlDegradationReason degradation(
	DegradationReason value)
{
	switch (value)
	{
	case DegradationReason::None:
		return Core::FlightControlDegradationReason::None;
	case DegradationReason::SustainedVerticalTrackingFailure:
		return Core::FlightControlDegradationReason::
			SustainedVerticalTrackingFailure;
	case DegradationReason::SustainedLateralTrackingFailure:
		return Core::FlightControlDegradationReason::
			SustainedLateralTrackingFailure;
	case DegradationReason::SustainedActuatorSaturation:
		return Core::FlightControlDegradationReason::
			SustainedActuatorSaturation;
	case DegradationReason::MagneticHeadingUnavailable:
		return Core::FlightControlDegradationReason::MagneticHeadingUnavailable;
	case DegradationReason::PressureAltitudeUnavailable:
		return Core::FlightControlDegradationReason::PressureAltitudeUnavailable;
	}
	throw std::logic_error("Unknown FLCC degradation reason.");
}

inline Core::FlightControlDisconnectReason disconnect(DisconnectReason value)
{
	switch (value)
	{
	case DisconnectReason::None:
		return Core::FlightControlDisconnectReason::None;
	case DisconnectReason::PilotCommand:
		return Core::FlightControlDisconnectReason::PilotCommand;
	case DisconnectReason::SafetyCondition:
		return Core::FlightControlDisconnectReason::SafetyCondition;
	case DisconnectReason::InvalidInput:
		return Core::FlightControlDisconnectReason::InvalidInput;
	}
	throw std::logic_error("Unknown FLCC disconnect reason.");
}

inline Core::AutomaticFlightControlReason automatic_reason(
	AutomaticFlightControlReason value)
{
	switch (value)
	{
	case AutomaticFlightControlReason::None:
		return Core::AutomaticFlightControlReason::None;
	case AutomaticFlightControlReason::Commanded:
		return Core::AutomaticFlightControlReason::Commanded;
	case AutomaticFlightControlReason::BelowMinimumIndicatedAirspeed:
		return Core::AutomaticFlightControlReason::
			BelowMinimumIndicatedAirspeed;
	case AutomaticFlightControlReason::WeightOnWheels:
		return Core::AutomaticFlightControlReason::WeightOnWheels;
	case AutomaticFlightControlReason::RollLimit:
		return Core::AutomaticFlightControlReason::RollLimit;
	case AutomaticFlightControlReason::PitchLimit:
		return Core::AutomaticFlightControlReason::PitchLimit;
	case AutomaticFlightControlReason::MachLimit:
		return Core::AutomaticFlightControlReason::MachLimit;
	case AutomaticFlightControlReason::MagneticHeadingUnavailable:
		return Core::AutomaticFlightControlReason::MagneticHeadingUnavailable;
	case AutomaticFlightControlReason::PressureAltitudeUnavailable:
		return Core::AutomaticFlightControlReason::PressureAltitudeUnavailable;
	}
	throw std::logic_error("Unknown automatic-flight-control reason.");
}
}
}
}
