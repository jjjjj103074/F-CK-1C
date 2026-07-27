#include "AircraftSimulation.h"

#include "../ForceMoment.h"

#include <stdexcept>
#include "../../Systems/FBWLifecycle.h"

namespace
{
constexpr double kColdStartThrottle = 0.0;
constexpr double kHotAirStartThrottle = 0.5;

Systems::StartupMode startup_mode(Core::StartMode mode)
{
	switch (mode)
	{
	case Core::StartMode::ColdGround:
		return Systems::STARTUP_MODE_COLD_GROUND;
	case Core::StartMode::HotGround:
		return Systems::STARTUP_MODE_HOT_GROUND;
	case Core::StartMode::HotAir:
		return Systems::STARTUP_MODE_HOT_AIR;
	}
	throw std::invalid_argument("Unknown Core::StartMode.");
}
}

namespace Core
{
namespace Simulation
{
void validate_aircraft_config(const Data::AircraftConfig& config)
{
	if (!Systems::has_valid_aerodynamics_tables(config.aerodynamics))
	{
		throw std::invalid_argument("Fck1cEfm requires complete aerodynamic tables.");
	}
	if (!Systems::has_valid_engine_tables(config.engine))
	{
		throw std::invalid_argument("Fck1cEfm requires complete engine tables.");
	}
}

AircraftSimulation::AircraftSimulation(
	const Data::AircraftConfig& config,
	const FlightSetupContext& setup)
	: config_(config)
{
	validate_aircraft_config(config_);
	apply_setup(setup);
}

void AircraftSimulation::apply_setup(const FlightSetupContext& setup)
{
	gameplay_.options = setup.options;
	Systems::set_internal_fuel(systems_.fuel, setup.fuel.internal_fuel);
	for (const auto& station : setup.fuel.external_fuel_by_station)
	{
		set_external_fuel(station.first, station.second);
	}
	Systems::reset_damage_model(systems_.damage);
	Systems::reset_suspension_feedback_state(systems_.suspension);
	Systems::reset_fbw_state(
		systems_.fbw,
		{
			aircraft_state_.roll,
			aircraft_state_.pitch,
			aircraft_state_.alpha,
			aircraft_state_.g
		});
	Systems::begin_startup(systems_.startup, startup_mode(setup.start_mode));
	configure_start_state(setup);
}

FrameOutput AircraftSimulation::initial_output() const
{
	return make_frame_output({});
}

double AircraftSimulation::internal_fuel() const
{
	return Systems::get_internal_fuel(systems_.fuel);
}

double AircraftSimulation::external_fuel() const
{
	return Systems::get_external_fuel(systems_.fuel);
}

FlightFuelLoad AircraftSimulation::fuel_load() const
{
	FlightFuelLoad load;
	load.internal_fuel = systems_.fuel.internal_fuel;
	for (const auto& station : systems_.fuel.external_fuel_by_station)
	{
		load.external_fuel_by_station[station.first] = {
			station.second.value,
			station.second.position
		};
	}
	return load;
}

MassDeltaResult AircraftSimulation::take_mass_delta()
{
	const Systems::FuelMassDeltaResult source = Systems::take_fuel_mass_delta(systems_.fuel);
	MassDeltaResult result;
	result.available = source.available;
	result.delta.mass = source.delta.mass;
	result.delta.position = source.delta.position;
	result.delta.moment_of_inertia = source.delta.moment_of_inertia;
	return result;
}

void AircraftSimulation::set_internal_fuel(double fuel)
{
	Systems::set_internal_fuel(systems_.fuel, fuel);
}

void AircraftSimulation::set_external_fuel(
	int station,
	const ExternalFuelLoad& fuel)
{
	Systems::set_external_fuel(
		systems_.fuel,
		{ station, fuel.fuel, fuel.position });
}

void AircraftSimulation::set_infinite_fuel(bool enabled)
{
	gameplay_.options.infinite_fuel = enabled;
}

void AircraftSimulation::set_easy_flight(bool enabled)
{
	gameplay_.options.easy_flight = enabled;
}

void AircraftSimulation::set_invincible(bool enabled)
{
	gameplay_.options.invincible = enabled;
}

DamageApplyResult AircraftSimulation::apply_damage(const DamageEvent& event)
{
	switch (event.area)
	{
	case DamageArea::LeftWing:
		Systems::apply_damage_segment(systems_.damage.left_wing_segments, event.segment, event.integrity);
		break;
	case DamageArea::RightWing:
		Systems::apply_damage_segment(systems_.damage.right_wing_segments, event.segment, event.integrity);
		break;
	case DamageArea::Tail:
		Systems::apply_damage_segment(systems_.damage.tail_segments, event.segment, event.integrity);
		break;
	case DamageArea::LeftEngine:
		Systems::apply_damage_segment(systems_.damage.left_engine_segments, event.segment, event.integrity);
		break;
	case DamageArea::RightEngine:
		Systems::apply_damage_segment(systems_.damage.right_engine_segments, event.segment, event.integrity);
		break;
	}
	if (!gameplay_.options.invincible)
	{
		Systems::refresh_damage_integrity(systems_.damage);
	}
	return { gameplay_.options.invincible };
}

void AircraftSimulation::apply_mass_input(const MassStateInput& input)
{
	Core::set_current_mass(aircraft_state_, input.mass);
	force_moment_.center_of_mass = input.center_of_mass;
}

void AircraftSimulation::apply_suspension_input(const FrameInput& input)
{
	for (std::size_t index = 0; index < input.suspension.size(); ++index)
	{
		if (!input.availability.suspension[index])
		{
			continue;
		}
		const SuspensionFeedbackInput& wheel = input.suspension[index];
		Systems::update_suspension_feedback(
			systems_.suspension,
			{ wheel.index, wheel.compression, wheel.acting_force });
	}
}

void AircraftSimulation::apply_frame_input(const FrameInput& input)
{
	if (input.availability.atmosphere)
	{
		Core::set_atmosphere(aircraft_state_, input.atmosphere);
	}
	if (input.availability.surface)
	{
		Core::set_surface(aircraft_state_, input.surface);
	}
	if (input.availability.mass)
	{
		apply_mass_input(input.mass);
	}
	if (input.availability.world_kinematics)
	{
		Core::set_world_kinematics(aircraft_state_, input.world_kinematics);
	}
	if (input.availability.body_kinematics)
	{
		Core::set_body_kinematics(aircraft_state_, input.body_kinematics);
	}
	apply_suspension_input(input);
}

FrameOutput AircraftSimulation::step(const FrameInput& input)
{
	apply_frame_input(input);
	begin_frame(input.dt_s);
	update_airframe(input.dt_s);
	Systems::update_primary_control_inputs(systems_.primary_controls);
	update_autopilot(input.autopilot);
	update_fbw(input.dt_s);
	const Systems::AerodynamicsFrameInput aerodynamics_input = make_aerodynamics_input();
	update_primary_aerodynamics(aerodynamics_input);
	update_engines_and_fuel(input.dt_s, input.max_power);
	update_ground_and_suspension(input.dt_s, aerodynamics_input);
	finish_frame();
	return make_frame_output(input.availability);
}

void AircraftSimulation::begin_frame(double dt)
{
	Systems::advance_simulation_time(systems_.startup, dt);
	reset_force_moment(force_moment_.force, force_moment_.moment);
	if (systems_.startup.first_frame_completed)
	{
		return;
	}

	Systems::initialize_aerodynamic_force_positions(
		systems_.aerodynamics,
		config_.aerodynamics,
		force_moment_.center_of_mass);
}

void AircraftSimulation::update_airframe(double dt)
{
	Systems::update_gear_position(systems_.landing_gear);
	const Systems::AirframeDeviceUpdateInput device_input = {
		aircraft_state_.speed_scalar,
		systems_.landing_gear.position
	};
	Systems::update_airframe_device_positions(
		systems_.airframe_devices,
		device_input);
	Systems::update_nose_wheel_steering(
		systems_.landing_gear.wheels,
		nose_wheel_steering());
	update_airspeed(aircraft_state_);
	const Systems::WheelSpinInput wheel_input = {
		ground_speed(aircraft_state_),
		dt,
		aircraft_state_.altitude_agl,
		{
			config_.suspension.fallback_wheel_radius[0],
			config_.suspension.fallback_wheel_radius[1],
			config_.suspension.fallback_wheel_radius[2]
		}
	};
	Systems::update_wheel_spin(
		systems_.landing_gear.wheels,
		systems_.landing_gear.position,
		wheel_input);
	Systems::update_aerodynamic_conditions(
		systems_.aerodynamics,
		config_.aerodynamics,
		{
			force_moment_.center_of_mass,
			aircraft_state_.atmosphere_density,
			aircraft_state_.speed_scalar,
			aircraft_state_.mach,
			aircraft_state_.alpha,
			aircraft_state_.beta,
			systems_.airframe_devices.slats_pos
		});
}

void AircraftSimulation::update_autopilot(const AutopilotCommand& command)
{
	if (command.master && !command.bypass)
	{
		systems_.primary_controls.pitch.input = command.pitch_command;
		systems_.primary_controls.roll.input = command.roll_command;
	}

	if (command.auto_throttle_engaged)
	{
		systems_.fbw.throttle_cmd_left = command.throttle_command;
		systems_.fbw.throttle_cmd_right = command.throttle_command;
		systems_.fbw.throttle_blend = 1.0;
		systems_.fbw.throttle_override = false;
		return;
	}

	systems_.fbw.throttle_blend = 0.0;
}

void AircraftSimulation::update_fbw(double dt)
{
	const Systems::FBWControllerOutput output = Systems::update_fbw_controller(
		systems_.fbw,
		config_.fbw,
		make_fbw_input(dt));
	control_surfaces_.elevator_command = output.elevator_command;
	control_surfaces_.aileron_command = output.aileron_command;
	control_surfaces_.rudder_command = output.rudder_command;
}

Systems::FBWControllerInput AircraftSimulation::make_fbw_input(double dt) const
{
	Systems::FBWControllerInput input;
	input.dt = dt;
	input.qbar = systems_.aerodynamics.dynamic_pressure;
	input.alpha_limit_deg = systems_.aerodynamics.alpha_max_deg;
	input.roll = aircraft_state_.roll;
	input.pitch = aircraft_state_.pitch;
	input.roll_rate = aircraft_state_.roll_rate;
	input.pitch_rate = aircraft_state_.pitch_rate;
	input.yaw_rate = aircraft_state_.yaw_rate;
	input.alpha = aircraft_state_.alpha;
	input.beta = aircraft_state_.beta;
	input.speed_scalar = aircraft_state_.speed_scalar;
	input.mach = aircraft_state_.mach;
	input.g = aircraft_state_.g;
	input.roll_input = systems_.primary_controls.roll.input;
	input.roll_trim = systems_.primary_controls.roll.trim;
	input.pitch_input = systems_.primary_controls.pitch.input;
	input.pitch_trim = systems_.primary_controls.pitch.trim;
	input.yaw_input = systems_.primary_controls.yaw.input;
	input.yaw_trim = systems_.primary_controls.yaw.trim;
	input.gear_pos = systems_.landing_gear.position;
	input.wow = Systems::has_suspension_feedback(systems_.suspension) &&
		Systems::any_wow(systems_.suspension);
	input.elevator_command = control_surfaces_.elevator_command;
	input.aileron_command = control_surfaces_.aileron_command;
	input.rudder_command = control_surfaces_.rudder_command;
	return input;
}

Systems::AerodynamicsFrameInput AircraftSimulation::make_aerodynamics_input() const
{
	Systems::AerodynamicsFrameInput input;
	input.center_of_mass = force_moment_.center_of_mass;
	input.mach = aircraft_state_.mach;
	input.aoa = aircraft_state_.aoa;
	input.alpha_deg = aircraft_state_.alpha;
	input.aos = aircraft_state_.aos;
	input.roll = aircraft_state_.roll;
	input.pitch_rate = aircraft_state_.pitch_rate;
	input.roll_rate = aircraft_state_.roll_rate;
	input.yaw_rate = aircraft_state_.yaw_rate;
	input.elevator_command = control_surfaces_.elevator_command;
	input.aileron_command = control_surfaces_.aileron_command;
	input.rudder_command = control_surfaces_.rudder_command;
	input.airbrake_pos = systems_.airframe_devices.airbrake_pos;
	input.flaps_pos = systems_.airframe_devices.flaps_pos;
	input.gear_pos = systems_.landing_gear.position;
	input.left_wing_integrity = systems_.damage.left_wing_integrity;
	input.right_wing_integrity = systems_.damage.right_wing_integrity;
	input.tail_integrity = systems_.damage.tail_integrity;
	input.easy_flight = gameplay_.options.easy_flight;
	return input;
}

void AircraftSimulation::update_primary_aerodynamics(
	const Systems::AerodynamicsFrameInput& input)
{
	Systems::apply_primary_aerodynamics(
		systems_.aerodynamics,
		{ config_.aerodynamics, input },
		[this](const Common::Vec3& force, const Common::Vec3& position)
		{
			add_force(force, position);
		});
}

void AircraftSimulation::update_engines_and_fuel(
	double dt,
	const MaxPowerCommand& max_power)
{
	const double dry_thrust = max_dry_thrust();
	Systems::update_pilot_throttle_cmds(systems_.throttle_inputs);
	Systems::apply_engine_throttle_commands(
		systems_.engines,
		Systems::compose_engine_throttle_cmd(
			{
				systems_.throttle_inputs.left.pilot_cmd,
				systems_.fbw.throttle_cmd_left,
				systems_.fbw.throttle_blend,
				systems_.fbw.throttle_override
			}),
		Systems::compose_engine_throttle_cmd(
			{
				systems_.throttle_inputs.right.pilot_cmd,
				systems_.fbw.throttle_cmd_right,
				systems_.fbw.throttle_blend,
				systems_.fbw.throttle_override
			}));
	update_engine_state(dt, dry_thrust);
	handle_engine_shutdown(dt);
	apply_thrust(max_power);
	update_fuel(dt);
}

double AircraftSimulation::max_dry_thrust() const
{
	return Systems::max_dry_thrust(config_.engine, aircraft_state_.mach);
}

void AircraftSimulation::update_engine_state(double dt, double dry_thrust)
{
	Systems::clamp_engine_throttle_inputs(systems_.engines);
	Systems::update_dry_engine_channels(
		systems_.engines,
		config_.engine,
		dt);
	Systems::update_afterburners(systems_.engines, config_.engine, dt);
	Systems::update_nozzle_apertures(systems_.engines, config_.engine, dt);
	Systems::update_engine_thrust_outputs(
		systems_.engines,
		config_.engine,
		{
			dry_thrust,
			aircraft_state_.engine_alt_effect,
			systems_.damage.left_engine_integrity,
			systems_.damage.right_engine_integrity
		});
	Systems::apply_engine_readout_integrity(
		systems_.engines,
		systems_.damage.left_engine_integrity,
		systems_.damage.right_engine_integrity);
}

void AircraftSimulation::handle_engine_shutdown(double dt)
{
	if (!Systems::should_shutdown_engines(systems_.fuel.internal_fuel, aircraft_state_.altitude_asl))
	{
		return;
	}

	Systems::shutdown_engines(systems_.engines, dt);
}

void AircraftSimulation::apply_thrust(const MaxPowerCommand& command)
{
	Systems::apply_thrust_cut(systems_.engines, command.ready > 0.5 && command.value < 0.5);
	add_force(Common::Vec3(systems_.engines.left.thrust_force, 0.0, 0.0), config_.left_engine_position);
	add_force(Common::Vec3(systems_.engines.right.thrust_force, 0.0, 0.0), config_.right_engine_position);
}

void AircraftSimulation::update_fuel(double dt)
{
	if (gameplay_.options.infinite_fuel)
	{
		systems_.fuel.total_fuel_flow = 0.0;
		return;
	}

	const Systems::FuelConsumptionInput input = {
		dt,
		systems_.engines.left.throttle_output,
		systems_.engines.right.throttle_output,
		systems_.engines.left.afterburner_ratio,
		systems_.engines.right.afterburner_ratio,
		config_.engine.afterburner.fuel_factor
	};
	Systems::simulate_fuel_consumption(systems_.fuel, config_.fuel, input);
}

void AircraftSimulation::update_ground_and_suspension(
	double dt,
	const Systems::AerodynamicsFrameInput& input)
{
	auto add_force_sink = [this](
		const Common::Vec3& force,
		const Common::Vec3& position)
	{
		add_force(force, position);
	};
	auto add_moment_sink = [this](const Common::Vec3& moment)
	{
		add_moment(moment);
	};
	Systems::apply_aerodynamic_limiters(
		systems_.aerodynamics,
		{ config_.aerodynamics, input },
		Systems::make_aerodynamic_sinks(add_force_sink, add_moment_sink));
	apply_fallback_ground_forces();
	Systems::update_on_ground(systems_.suspension, systems_.landing_gear.position);
	Systems::AerodynamicsFrameInput shake_input = input;
	shake_input.on_ground = systems_.suspension.on_ground;
	shake_input.g_force = aircraft_state_.g;
	gameplay_.shake_amplitude = Systems::update_aerodynamic_shake(
		systems_.aerodynamics,
		config_.aerodynamics,
		shake_input);
}

void AircraftSimulation::apply_fallback_ground_forces()
{
	const Systems::SuspensionFallbackInput input = {
		aircraft_state_.altitude_agl,
		aircraft_state_.pitch,
		aircraft_state_.roll,
		aircraft_state_.velocity_world.y,
		aircraft_state_.velocity_body.x,
		systems_.landing_gear.position,
		aircraft_state_.current_mass,
		systems_.engines.left.throttle_input,
		systems_.engines.right.throttle_input,
		systems_.engines.left.thrust_force,
		systems_.engines.right.thrust_force,
		systems_.landing_gear.wheels.brake_left,
		systems_.landing_gear.wheels.brake_right
	};
	systems_.suspension.fallback_ground_force = Systems::apply_fallback_ground_forces(
		systems_.suspension,
		Systems::make_suspension_fallback_context(config_.suspension, input),
		[this](const Common::Vec3& force, const Common::Vec3& position)
		{
			add_force(force, position);
		});
}

double AircraftSimulation::nose_wheel_steering() const
{
	return Systems::compute_nose_wheel_steering(
		systems_.landing_gear,
		aircraft_state_.speed_scalar,
		systems_.primary_controls.yaw.input);
}

void AircraftSimulation::add_force(
	const Common::Vec3& force,
	const Common::Vec3& position)
{
	Core::add_local_force(
		force_moment_.force,
		force_moment_.moment,
		{ force_moment_.center_of_mass, force, position });
}

void AircraftSimulation::add_moment(const Common::Vec3& moment)
{
	Core::add_local_moment(force_moment_.moment, moment);
}

void AircraftSimulation::finish_frame()
{
	Systems::mark_first_frame_completed(systems_.startup);
}

void AircraftSimulation::configure_start_state(
	const FlightSetupContext& setup)
{
	switch (setup.start_mode)
	{
	case StartMode::ColdGround:
		Systems::configure_ground_start_landing_gear(systems_.landing_gear);
		Systems::reset_throttle_inputs(
			systems_.throttle_inputs, kColdStartThrottle, kColdStartThrottle);
		Systems::configure_cold_start_engines(systems_.engines, config_.engine);
		return;
	case StartMode::HotGround:
		Systems::configure_ground_start_landing_gear(systems_.landing_gear);
		Systems::configure_hot_ground_start_devices(systems_.airframe_devices);
		Systems::reset_throttle_inputs(
			systems_.throttle_inputs, kColdStartThrottle, kColdStartThrottle);
		Systems::configure_hot_ground_start_engines(systems_.engines, config_.engine);
		return;
	case StartMode::HotAir:
		Systems::configure_air_start_landing_gear(systems_.landing_gear);
		Systems::reset_throttle_inputs(
			systems_.throttle_inputs, kHotAirStartThrottle, kHotAirStartThrottle);
		Systems::configure_hot_air_start_engines(systems_.engines, config_.engine);
		return;
	}
}

void AircraftSimulation::repair(const RepairEvent& event)
{
	(void)event;
	Systems::reset_damage_model(systems_.damage);
}
}
}
