#include "Fck1cEfm.h"

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

Core::EngineOutput project_engine(const Systems::EngineChannelState& source)
{
	return {
		source.switch_on,
		source.throttle_input,
		source.throttle_output,
		source.power_readout,
		source.thrust_force,
		source.afterburner_ratio,
		source.afterburner_lit,
		source.nozzle_aperture
	};
}

Core::ControlOutput project_controls(
	const Systems::PrimaryControlState& input,
	const Core::ControlSurfaceState& surfaces,
	const Systems::AirframeDeviceState& devices)
{
	return {
		input.pitch.input,
		input.roll.input,
		input.yaw.input,
		surfaces.elevator_command,
		surfaces.aileron_command,
		surfaces.rudder_command,
		devices.flaps_pos,
		devices.slats_pos,
		devices.airbrake_pos
	};
}

Core::LandingGearOutput project_landing_gear(
	const Systems::LandingGearSystemState& source)
{
	Core::LandingGearOutput output;
	output.gear_position = source.position;
	output.nose_wheel_steering = source.wheels.nose_steering;
	output.brake_left = source.wheels.brake_left;
	output.brake_right = source.wheels.brake_right;
	for (std::size_t index = 0; index < output.wheel_spin.size(); ++index)
	{
		output.wheel_spin[index] = source.wheels.spin[index];
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

Core::FuelOutput project_fuel(const Systems::FuelSystem& source)
{
	return {
		source.internal_fuel,
		source.external_fuel,
		source.total_fuel,
		source.total_fuel_flow
	};
}
}

namespace Core
{
FrameOutput Fck1cEfm::make_frame_output(
	const FrameDataAvailability& availability) const
{
	FrameOutput output;
	output.simulation_time_s = systems_.startup.simulation_time;
	output.availability = availability;
	output.flight = project_flight(aircraft_state_);
	output.force_moment = {
		force_moment_.force,
		force_moment_.moment,
		force_moment_.center_of_mass
	};
	output.engines = {
		project_engine(systems_.engines.left),
		project_engine(systems_.engines.right)
	};
	output.controls = project_controls(
		systems_.primary_controls,
		control_surfaces_,
		systems_.airframe_devices);
	output.landing_gear = project_landing_gear(systems_.landing_gear);
	output.suspension = project_suspension(systems_.suspension);
	output.fuel = project_fuel(systems_.fuel);
	output.shake_amplitude = gameplay_.shake_amplitude;
	return output;
}
}
