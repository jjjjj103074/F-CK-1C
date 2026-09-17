#include "AircraftSimulation.h"

namespace
{
Core::FlightOutput project_flight(const Core::AircraftState& source)
{
	return {
		source.altitude_asl_m,
		source.altitude_agl_m,
		source.position_world_z_m,
		source.mach,
		source.normal_acceleration_g,
		source.angle_of_attack_deg,
		source.angle_of_slide_deg,
		source.atmosphere_temperature_k,
		indicated_airspeed_mps(source),
		source.velocity_world_mps.y,
		source.world_yaw_rad,
		source.pitch_rad,
		source.roll_rad,
		source.roll_rate_rad_s,
		source.pitch_rate_rad_s,
		source.yaw_rate_rad_s
	};
}

Core::EngineOutput project_engine(
	const Core::EngineChannelData& source,
	double thrust_force_n)
{
	return {
		source.switch_on,
		source.throttle_input_normalized,
		source.throttle_output_normalized,
		source.power_readout_normalized,
		thrust_force_n,
		source.afterburner_ratio_0_1,
		source.afterburner_lit,
		source.nozzle_aperture_normalized
	};
}

Core::ControlOutput project_controls(
	const Core::PilotControlSignal& input,
	const Core::FlightControlActuatorState& primary,
	const Core::SecondaryControlPosition& secondary)
{
	return {
		input.pitch_axis_normalized,
		input.roll_axis_normalized,
		input.yaw_axis_normalized,
		primary.symmetric_stabilator.position_rad,
		primary.differential_flaperon.position_rad,
		primary.rudder.position_rad,
		secondary.flaps_position_normalized,
		secondary.slats_position_normalized,
		secondary.airbrake_position_normalized
	};
}

Core::LandingGearOutput project_landing_gear(
	const Core::LandingGearData& source)
{
	Core::LandingGearOutput output;
	output.gear_position_normalized = source.position_normalized;
	output.nose_wheel_steering_normalized =
		source.nose_wheel_steering_normalized;
	output.brake_left_normalized = source.brake_left_normalized;
	output.brake_right_normalized = source.brake_right_normalized;
	for (std::size_t index = 0;
		index < output.wheel_spin_phase_0_1.size();
		++index)
	{
		output.wheel_spin_phase_0_1[index] =
			source.wheel_spin_phase_0_1[index];
	}
	return output;
}

Core::SuspensionOutput project_suspension(
	const Core::LandingGearData& source)
{
	Core::SuspensionOutput output;
	for (std::size_t index = 0; index < output.wheels.size(); ++index)
	{
		Core::SuspensionWheelOutput& wheel = output.wheels[index];
		const Core::SuspensionWheelData& source_wheel =
			source.suspension[index];
		wheel.acting_force_body_n = source_wheel.acting_force_body_n;
		wheel.compression_m = source_wheel.compression_m;
		wheel.force_magnitude_n = source_wheel.force_magnitude_n;
		wheel.weight_on_wheel = source_wheel.weight_on_wheel;
	}
	output.any_weight_on_wheels = source.any_weight_on_wheels;
	output.on_ground = source.on_ground;
	return output;
}

Core::FuelOutput project_fuel(const Core::FuelData& source)
{
	return {
		source.internal_fuel_kg,
		source.external_fuel_kg,
		source.internal_fuel_kg + source.external_fuel_kg,
		source.total_fuel_flow_kg_s
	};
}
}

namespace Core
{
namespace Simulation
{
FrameOutput AircraftSimulation::make_frame_output(
	const Systems::AircraftDataSnapshot& aircraft,
	const SimulationResult& simulation,
	const FrameDataAvailability& availability) const
{
	const EngineData& engines =
		aircraft.read(AircraftDataKeys::kEngineData);
	const LandingGearData& landing_gear =
		aircraft.read(AircraftDataKeys::kLandingGearData);
	FrameOutput output;
	output.simulation_time_s = simulation_time_s_;
	output.availability = availability;
	output.flight = project_flight(aircraft_state_);
	output.force_moment = simulation.force_moment;
	output.engines = {
		project_engine(engines.left, simulation.thrust_force_n[0]),
		project_engine(engines.right, simulation.thrust_force_n[1])
	};
	output.controls = project_controls(
		aircraft.read(AircraftDataKeys::kPilotControlSignal),
		aircraft.read(AircraftDataKeys::kFlightControlActuatorState),
		aircraft.read(AircraftDataKeys::kSecondaryControlPosition));
	output.landing_gear = project_landing_gear(landing_gear);
	output.suspension = project_suspension(landing_gear);
	output.fuel = project_fuel(
		aircraft.read(AircraftDataKeys::kFuelData));
	const PropulsionTestIntent& diagnostics =
		aircraft.read(AircraftDataKeys::kPropulsionTestIntent);
	output.propulsion_diagnostics.thrust_cut_requested =
		diagnostics.thrust_cut_requested;
	output.mass_effect = simulation.mass_effect;
	output.cockpit.status = { true, cockpit_snapshot_revision_ };
	output.cockpit.simulation_time_s = simulation_time_s_;
	output.cockpit.flight_control_computer =
		aircraft.read(AircraftDataKeys::kFlightControlComputerSnapshot);
	output.cockpit.automatic_flight_control =
		aircraft.read(AircraftDataKeys::kAutomaticFlightControlSnapshot);
	output.cockpit.propulsion_test_thrust_cut_requested =
		diagnostics.thrust_cut_requested;
	output.shake_amplitude_normalized = simulation.shake_amplitude_normalized;
	return output;
}
}
}
