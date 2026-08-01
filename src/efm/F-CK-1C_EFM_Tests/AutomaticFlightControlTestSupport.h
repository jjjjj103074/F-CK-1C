#pragma once

#include "Core/Systems/FlightControlComputer/Autopilot/AutomaticFlightControl.h"
#include "Core/Systems/FlightControlComputer/ControlLaws/ConfigurationAndMode.h"

namespace AutomaticFlightControlTestSupport
{
using Core::AutomaticFlightControlLateralMode;
using Core::AutomaticFlightControlReason;
using Core::AutomaticFlightControlVerticalMode;
using Core::CommandId;
using Core::Systems::AutomaticFlightControl;
using Core::Systems::AutomaticFlightControlObservation;

constexpr double kTolerance = 1e-9;
constexpr double kReferenceDtS = 0.02;
constexpr double kMetersPerSecondPerKnot = 0.5144444444444445;
constexpr double kNominalIasKts = 300.0;
constexpr double kNominalAltitudeM = 2000.0;
constexpr double kNominalMach = 0.7;
constexpr double kNominalHeadingRad = 1.0;
constexpr double kNominalPitchRad = 0.1;
constexpr double kNominalRollRad = -0.1;
constexpr double kTestAlphaLimitDeg = 20.0;

inline double knots(double value)
{
	return value * kMetersPerSecondPerKnot;
}

inline AutomaticFlightControlObservation nominal_observation()
{
	AutomaticFlightControlObservation result;
	result.dt_s = kReferenceDtS;
	result.indicated_airspeed_mps = knots(kNominalIasKts);
	result.altitude_m = kNominalAltitudeM;
	result.mach = kNominalMach;
	result.heading_rad = kNominalHeadingRad;
	result.pitch_rad = kNominalPitchRad;
	result.roll_rad = kNominalRollRad;
	return result;
}

inline AutomaticFlightControl make_control(bool initial_wow = false)
{
	return AutomaticFlightControl(
		Core::Systems::fck1c_automatic_flight_control_config(),
		initial_wow);
}

inline const ::Systems::ManeuverEnvelope& nominal_envelope()
{
	static const ::Systems::FBWControllerConfig config;
	static const ::Systems::ManeuverEnvelope envelope =
		::Systems::make_maneuver_envelope(
			config, { config.cat1, kTestAlphaLimitDeg, false });
	return envelope;
}

inline const Core::Systems::AutomaticFlightGuidanceReference& step(
	AutomaticFlightControl& control,
	const AutomaticFlightControlObservation& observation)
{
	return control.step(observation, nominal_envelope());
}

inline void send(
	AutomaticFlightControl& control,
	CommandId id,
	double value = 1.0)
{
	control.handle_command({ id, value });
}

inline void prime_and_engage(
	AutomaticFlightControl& control,
	const AutomaticFlightControlObservation& observation)
{
	(void)step(control, observation);
	send(control, CommandId::EngageAutopilot);
	(void)step(control, observation);
}
}
