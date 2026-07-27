#include "AircraftSimulation.h"

namespace
{
static_assert(
	Core::kFrameSuspensionWheelCount == Systems::kSuspensionWheelCount,
	"Frame and suspension wheel counts must match.");
static_assert(
	Core::kFrameSuspensionWheelCount == Systems::kLandingGearWheelCount,
	"Frame and landing gear wheel counts must match.");

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
	const Systems::SuspensionSystemState& source)
{
	Core::SuspensionOutput output;
	for (std::size_t index = 0; index < output.wheels.size(); ++index)
	{
		Core::SuspensionWheelOutput& wheel = output.wheels[index];
		wheel.acting_force = source.force_vec[index];
		wheel.compression = source.compression[index];
		wheel.force_magnitude = source.force_mag[index];
		wheel.weight_on_wheel = source.wow[index];
	}
	output.any_weight_on_wheels = Systems::any_wow(source);
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
	const FrameDataAvailability& availability) const
{
	FrameOutput output;
	output.simulation_time_s = startup_.simulation_time;
	output.availability = availability;
	output.flight = project_flight(aircraft_state_);
	output.force_moment = {
		force_moment_.force,
		force_moment_.moment,
		force_moment_.center_of_mass
	};
	output.engines = {
		project_engine(engine_.data().left, left_thrust_force_),
		project_engine(engine_.data().right, right_thrust_force_)
	};
	output.controls = project_controls(
		flight_control_computer_.pilot_controls(),
		primary_flight_controls_.position(),
		secondary_flight_controls_.position());
	output.landing_gear = project_landing_gear(landing_gear_.data());
	output.suspension =
		project_suspension(landing_gear_.suspension_state());
	output.fuel = project_fuel(fuel_.data());
	output.shake_amplitude = gameplay_.shake_amplitude;
	return output;
}
}
}
