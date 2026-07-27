#include "AircraftSimulation.h"

namespace
{
Core::FlightOutput project_flight(const Core::AircraftState& source)
{
	return {
		source.altitude_asl,
		source.altitude_agl,
		source.position_world_z,
		source.mach,
		source.g,
		source.alpha,
		source.beta,
		source.atmosphere_temperature
	};
}

Core::EngineOutput project_engine(
	const Core::EngineChannelData& source,
	double thrust_force)
{
	return {
		source.switch_on,
		source.throttle_input,
		source.throttle_output,
		source.power_readout,
		thrust_force,
		source.afterburner_ratio,
		source.afterburner_lit,
		source.nozzle_aperture
	};
}

Core::ControlOutput project_controls(
	const Core::PilotControlState& input,
	const Core::PrimaryControlPosition& primary,
	const Core::SecondaryControlPosition& secondary)
{
	return {
		input.pitch,
		input.roll,
		input.yaw,
		primary.elevator,
		primary.aileron,
		primary.rudder,
		secondary.flaps,
		secondary.slats,
		secondary.airbrake
	};
}

Core::LandingGearOutput project_landing_gear(
	const Core::LandingGearData& source)
{
	Core::LandingGearOutput output;
	output.gear_position = source.position;
	output.nose_wheel_steering = source.nose_wheel_steering;
	output.brake_left = source.brake_left;
	output.brake_right = source.brake_right;
	for (std::size_t index = 0; index < output.wheel_spin.size(); ++index)
	{
		output.wheel_spin[index] = source.wheel_spin[index];
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
		wheel.acting_force = source_wheel.acting_force;
		wheel.compression = source_wheel.compression;
		wheel.force_magnitude = source_wheel.force_magnitude;
		wheel.weight_on_wheel = source_wheel.weight_on_wheel;
	}
	output.any_weight_on_wheels = source.any_weight_on_wheels;
	output.on_ground = source.on_ground;
	return output;
}

Core::FuelOutput project_fuel(const Core::FuelData& source)
{
	return {
		source.internal_fuel,
		source.external_fuel,
		source.internal_fuel + source.external_fuel,
		source.total_fuel_flow
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
	output.simulation_time_s = startup_.simulation_time;
	output.availability = availability;
	output.flight = project_flight(aircraft_state_);
	output.force_moment = simulation.force_moment;
	output.engines = {
		project_engine(engines.left, simulation.thrust_force[0]),
		project_engine(engines.right, simulation.thrust_force[1])
	};
	output.controls = project_controls(
		aircraft.read(AircraftDataKeys::kPilotControlState),
		aircraft.read(AircraftDataKeys::kPrimaryControlPosition),
		aircraft.read(AircraftDataKeys::kSecondaryControlPosition));
	output.landing_gear = project_landing_gear(landing_gear);
	output.suspension = project_suspension(landing_gear);
	output.fuel = project_fuel(
		aircraft.read(AircraftDataKeys::kFuelData));
	output.mass_effect = simulation.mass_effect;
	output.shake_amplitude = simulation.shake_amplitude;
	return output;
}
}
}
