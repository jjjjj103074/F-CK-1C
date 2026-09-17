#pragma once

#include "Core/Systems/FlightControlComputer/CommandSystem/FlightControlCommandSystem.h"
#include "Core/Systems/FlightControlComputer/Configuration/FlightControlComputerConfig.h"
#include "Common/Clamp.h"
#include "Common/Units.h"

#include <cmath>

namespace AutomaticFlightControlTestSupport
{
using Core::AutomaticFlightControlLateralMode;
using Core::AutomaticFlightControlReason;
using Core::AutomaticFlightControlVerticalMode;
using Core::CommandId;

constexpr double kTolerance = 1e-9;
constexpr double kReferenceDtS = 0.02;
constexpr double kMetersPerSecondPerKnot = 0.5144444444444445;
constexpr double kNominalIasKts = 300.0;
constexpr double kNominalAltitudeFt = 6561.679790026246;
constexpr double kNominalMach = 0.7;
constexpr double kNominalHeadingDeg = 57.29577951308232;
constexpr double kNominalPitchRad = 0.1;
constexpr double kNominalRollRad = -0.1;
constexpr double kTestAlphaLimitDeg = 20.0;
constexpr double kNominalDynamicPressurePa = 5000.0;

struct AutomaticFlightControlObservation
{
	double dt_s = 0.0;
	double indicated_airspeed_mps = 0.0;
	double pressure_altitude_ft = 0.0;
	double vertical_speed_ft_s = 0.0;
	double mach = 0.0;
	bool magnetic_heading_available = false;
	double magnetic_heading_deg = 0.0;
	double pitch_rad = 0.0;
	double roll_rad = 0.0;
	double conditioned_pitch_input_normalized = 0.0;
	double conditioned_roll_input_normalized = 0.0;
	bool weight_on_wheels = false;
	bool pressure_altitude_available = false;
};

inline double knots(double value)
{
	return value * kMetersPerSecondPerKnot;
}

inline AutomaticFlightControlObservation nominal_observation()
{
	AutomaticFlightControlObservation result;
	result.dt_s = kReferenceDtS;
	result.indicated_airspeed_mps = knots(kNominalIasKts);
	result.pressure_altitude_ft = kNominalAltitudeFt;
	result.pressure_altitude_available = true;
	result.mach = kNominalMach;
	result.magnetic_heading_available = true;
	result.magnetic_heading_deg = kNominalHeadingDeg;
	result.pitch_rad = kNominalPitchRad;
	result.roll_rad = kNominalRollRad;
	return result;
}

class AutomaticFlightControl final
{
public:
	AutomaticFlightControl(
		const Core::Systems::AutomaticFlightControlConfig& automatic_config,
		bool initial_weight_on_wheels,
		bool experimental_auto_throttle_available = false)
		: config_(make_config(
			automatic_config, experimental_auto_throttle_available)),
		  scheduling_(config_.mode_and_gain),
		  command_system_(config_, initial_weight_on_wheels)
	{
	}

	void handle_command(const Core::Command& command)
	{
		command_system_.handle_command(command);
	}

	const Core::Systems::AutomaticFlightGuidanceReference& step(
		const AutomaticFlightControlObservation& observation)
	{
		Core::Systems::FlightControlCommandSystemInput input;
		input.flight = make_flight_state(observation);
		input.signals = make_signals(observation);
		input.configuration = scheduling_.update({
			observation.dt_s, kNominalDynamicPressurePa,
			observation.mach, true, false, 0.0 });
		return command_system_.update(input).automatic;
	}

	void observe_control_result(
		const Core::Systems::FlightControlCommandMonitorInput& observation)
	{
		command_system_.observe_control_result(observation);
	}

	const Core::AutomaticFlightControlSnapshot& snapshot() const
	{
		return command_system_.automatic_snapshot();
	}

private:
	static Core::Systems::FlightControlComputerConfig make_config(
		const Core::Systems::AutomaticFlightControlConfig& automatic_config,
		bool experimental_auto_throttle_available)
	{
		auto result = Core::Systems::fck1c_flight_control_computer_config();
		result.automatic_flight_control = automatic_config;
		result.development.experimental_auto_throttle_available =
			experimental_auto_throttle_available;
		return result;
	}

	static ::Systems::ComputedFlightState make_flight_state(
		const AutomaticFlightControlObservation& observation)
	{
		::Systems::ComputedFlightState result;
		result.dt_s = observation.dt_s;
		result.dynamic_pressure_pa = kNominalDynamicPressurePa;
		result.pressure_altitude_ft = observation.pressure_altitude_ft;
		result.vertical_speed_ft_s = observation.vertical_speed_ft_s;
		if (observation.indicated_airspeed_mps > 0.0)
		{
			result.flight_path_angle_available = true;
			result.flight_path_angle_rad = std::asin(Common::limit(
				Common::metres(observation.vertical_speed_ft_s) /
					observation.indicated_airspeed_mps,
				-1.0, 1.0));
		}
		result.pressure_altitude_available =
			observation.pressure_altitude_available;
		result.magnetic_heading_available =
			observation.magnetic_heading_available;
		result.magnetic_heading_deg = observation.magnetic_heading_deg;
		result.pitch_attitude_rad = observation.pitch_rad;
		result.roll_attitude_rad = observation.roll_rad;
		result.indicated_airspeed_mps = observation.indicated_airspeed_mps;
		result.mach = observation.mach;
		result.normal_acceleration_g = 1.0;
		return result;
	}

	static ::Systems::ManagedFlightControlSignals make_signals(
		const AutomaticFlightControlObservation& observation)
	{
		::Systems::ManagedFlightControlSignals result;
		result.dt_s = observation.dt_s;
		result.pilot_pitch_normalized =
			observation.conditioned_pitch_input_normalized;
		result.pilot_roll_normalized =
			observation.conditioned_roll_input_normalized;
		result.weight_on_wheels = observation.weight_on_wheels;
		return result;
	}

	const Core::Systems::FlightControlComputerConfig config_;
	::Systems::ModeAndGainScheduling scheduling_;
	Core::Systems::FlightControlCommandSystem command_system_;
};

inline AutomaticFlightControl make_control(bool initial_wow = false)
{
	return AutomaticFlightControl(
		Core::Systems::fck1c_automatic_flight_control_config(),
		initial_wow,
		true);
}

inline const Core::Systems::AutomaticFlightGuidanceReference& step(
	AutomaticFlightControl& control,
	const AutomaticFlightControlObservation& observation)
{
	return control.step(observation);
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
