#include "AircraftSimulation.h"

#include "../ForceMoment.h"

#include <stdexcept>

namespace
{
constexpr double kMaxPowerReadyThreshold = 0.5;
constexpr double kMaxPowerCutThreshold = 0.5;

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
	if (!::Systems::has_valid_aerodynamics_tables(config.aerodynamics))
	{
		throw std::invalid_argument("Fck1cEfm requires complete aerodynamic tables.");
	}
	if (!::Systems::has_valid_engine_tables(config.engine))
	{
		throw std::invalid_argument("Fck1cEfm requires complete engine tables.");
	}
}

AircraftSimulation::AircraftSimulation(
	const Data::AircraftConfig& config,
	const FlightSetupContext& setup)
	: config_(config),
	flight_control_computer_(
		config.fbw,
		{
			config.aerodynamics.mach_table,
			config.aerodynamics.alpha_max_table
		},
		setup.start_mode),
	secondary_flight_controls_(setup.start_mode),
	landing_gear_(setup.start_mode, config.suspension),
	engine_(
		config.engine,
		config.fuel.consumption_rate,
		setup.start_mode)
{
	validate_aircraft_config(config_);
	apply_setup(setup);
}

void AircraftSimulation::apply_setup(const FlightSetupContext& setup)
{
	gameplay_.options = setup.options;
	fuel_.set_internal_fuel(setup.fuel.internal_fuel);
	for (const auto& station : setup.fuel.external_fuel_by_station)
	{
		set_external_fuel(station.first, station.second);
	}
	::Systems::begin_startup(startup_, startup_mode(setup.start_mode));
}

FrameOutput AircraftSimulation::initial_output() const
{
	return make_frame_output({});
}

double AircraftSimulation::internal_fuel() const
{
	return fuel_.internal_fuel();
}

double AircraftSimulation::external_fuel() const
{
	return fuel_.external_fuel();
}

FlightFuelLoad AircraftSimulation::fuel_load() const
{
	FlightFuelLoad load;
	load.internal_fuel = fuel_.state().internal_fuel;
	for (const auto& station : fuel_.state().external_fuel_by_station)
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
	const ::Systems::FuelMassDeltaResult source = fuel_.take_mass_delta();
	MassDeltaResult result;
	result.available = source.available;
	result.delta.mass = source.delta.mass;
	result.delta.position = source.delta.position;
	result.delta.moment_of_inertia = source.delta.moment_of_inertia;
	return result;
}

void AircraftSimulation::set_internal_fuel(double fuel)
{
	fuel_.set_internal_fuel(fuel);
}

void AircraftSimulation::set_external_fuel(
	int station,
	const ExternalFuelLoad& fuel)
{
	fuel_.set_external_fuel({ station, fuel.fuel, fuel.position });
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
	if (gameplay_.options.invincible)
	{
		return { true };
	}
	switch (event.area)
	{
	case DamageArea::LeftWing:
	case DamageArea::RightWing:
	case DamageArea::Tail:
		airframe_structure_.apply_damage(event); break;
	case DamageArea::LeftEngine:
	case DamageArea::RightEngine:
		engine_.apply_damage(event); break;
	}
	return { false };
}

void AircraftSimulation::apply_suspension_input(const FrameInput& input)
{
	landing_gear_.apply_suspension_feedback(input);
}

void AircraftSimulation::apply_frame_input(const FrameInput& input)
{
	Core::apply_aircraft_observations(aircraft_state_, input);
	if (input.availability.mass)
	{
		force_moment_.center_of_mass = input.mass.center_of_mass;
	}
	apply_suspension_input(input);
}

FrameOutput AircraftSimulation::step(const FrameInput& input)
{
	apply_frame_input(input);
	begin_frame(input.dt_s);
	update_airframe(input.dt_s);
	update_fbw(input.dt_s, input.autopilot);
	const ::Systems::AerodynamicsFrameInput aerodynamics_input =
		make_aerodynamics_input();
	update_primary_aerodynamics(aerodynamics_input);
	update_engines_and_fuel(input.dt_s, input.max_power);
	update_ground_and_suspension(input.dt_s, aerodynamics_input);
	finish_frame();
	return make_frame_output(input.availability);
}

void AircraftSimulation::begin_frame(double dt)
{
	::Systems::advance_simulation_time(startup_, dt);
	reset_force_moment(force_moment_.force, force_moment_.moment);
	if (startup_.first_frame_completed)
	{
		return;
	}

	::Systems::initialize_aerodynamic_force_positions(
		aerodynamics_,
		config_.aerodynamics,
		force_moment_.center_of_mass);
}

void AircraftSimulation::update_airframe(double dt)
{
	const Systems::LandingGearFrameInput landing_input = {
		aircraft_state_.speed_scalar,
		ground_speed(aircraft_state_),
		dt,
		aircraft_state_.altitude_agl,
		flight_control_computer_.pilot_controls().yaw
	};
	landing_gear_.step(landing_input);
	secondary_flight_controls_.step(
		aircraft_state_.speed_scalar,
		landing_gear_.data().position);
	update_airspeed(aircraft_state_);
	::Systems::update_aerodynamic_conditions(
		aerodynamics_,
		config_.aerodynamics,
		{
			force_moment_.center_of_mass,
			aircraft_state_.atmosphere_density,
			aircraft_state_.speed_scalar,
			aircraft_state_.mach,
			aircraft_state_.alpha,
			aircraft_state_.beta,
			secondary_flight_controls_.position().slats
		});
}

void AircraftSimulation::update_fbw(
	double dt,
	const AutopilotCommand& autopilot)
{
	const FlightControlDemand& demand = flight_control_computer_.step(
		make_fbw_input(dt),
		autopilot);
	primary_flight_controls_.step(demand);
}

::Systems::FBWControllerInput AircraftSimulation::make_fbw_input(
	double dt) const
{
	::Systems::FBWControllerInput input;
	input.dt = dt;
	input.qbar = aerodynamics_.dynamic_pressure;
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
	input.gear_pos = landing_gear_.data().position;
	input.wow =
		::Systems::has_suspension_feedback(
			landing_gear_.suspension_state()) &&
		::Systems::any_wow(landing_gear_.suspension_state());
	input.elevator_command =
		primary_flight_controls_.position().elevator;
	input.aileron_command =
		primary_flight_controls_.position().aileron;
	input.rudder_command =
		primary_flight_controls_.position().rudder;
	return input;
}

::Systems::AerodynamicsFrameInput
	AircraftSimulation::make_aerodynamics_input() const
{
	::Systems::AerodynamicsFrameInput input;
	input.center_of_mass = force_moment_.center_of_mass;
	input.mach = aircraft_state_.mach;
	input.aoa = aircraft_state_.aoa;
	input.alpha_deg = aircraft_state_.alpha;
	input.aos = aircraft_state_.aos;
	input.roll = aircraft_state_.roll;
	input.pitch_rate = aircraft_state_.pitch_rate;
	input.roll_rate = aircraft_state_.roll_rate;
	input.yaw_rate = aircraft_state_.yaw_rate;
	input.elevator_command =
		primary_flight_controls_.position().elevator;
	input.aileron_command =
		primary_flight_controls_.position().aileron;
	input.rudder_command =
		primary_flight_controls_.position().rudder;
	input.airbrake_pos =
		secondary_flight_controls_.position().airbrake;
	input.flaps_pos = secondary_flight_controls_.position().flaps;
	input.gear_pos = landing_gear_.data().position;
	input.left_wing_integrity =
		airframe_structure_.integrity().left_wing;
	input.right_wing_integrity =
		airframe_structure_.integrity().right_wing;
	input.tail_integrity = airframe_structure_.integrity().tail;
	input.easy_flight = gameplay_.options.easy_flight;
	return input;
}

void AircraftSimulation::update_primary_aerodynamics(
	const ::Systems::AerodynamicsFrameInput& input)
{
	::Systems::apply_primary_aerodynamics(
		aerodynamics_,
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
	engine_.step({
		dt,
		flight_control_computer_.engine_demand(),
		fuel_.internal_fuel(),
		aircraft_state_.altitude_asl
	});
	update_engine_thrust(dry_thrust);
	apply_thrust(max_power);
	update_fuel(dt);
}

double AircraftSimulation::max_dry_thrust() const
{
	return ::Systems::max_dry_thrust(
		config_.engine, aircraft_state_.mach);
}

void AircraftSimulation::update_engine_thrust(double dry_thrust)
{
	::Systems::EngineSystemState physical_state = engine_.state();
	::Systems::update_engine_thrust_outputs(
		physical_state,
		config_.engine,
		{
			dry_thrust,
			aircraft_state_.engine_alt_effect,
			engine_.left_integrity(),
			engine_.right_integrity()
		});
	left_thrust_force_ = physical_state.left.thrust_force;
	right_thrust_force_ = physical_state.right.thrust_force;
	if (engine_.data().thrust_inhibited)
	{
		left_thrust_force_ = 0.0;
		right_thrust_force_ = 0.0;
	}
}

void AircraftSimulation::apply_thrust(const MaxPowerCommand& command)
{
	const bool cut =
		command.ready > kMaxPowerReadyThreshold &&
		command.value < kMaxPowerCutThreshold;
	if (cut)
	{
		left_thrust_force_ = 0.0;
		right_thrust_force_ = 0.0;
	}
	add_force(
		Common::Vec3(left_thrust_force_, 0.0, 0.0),
		config_.left_engine_position);
	add_force(
		Common::Vec3(right_thrust_force_, 0.0, 0.0),
		config_.right_engine_position);
}

void AircraftSimulation::update_fuel(double dt)
{
	if (gameplay_.options.infinite_fuel)
	{
		fuel_.set_reported_flow(0.0);
		return;
	}
	fuel_.step(engine_.fuel_demand(), dt);
}

void AircraftSimulation::update_ground_and_suspension(
	double dt,
	const ::Systems::AerodynamicsFrameInput& input)
{
	(void)dt;
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
	::Systems::apply_aerodynamic_limiters(
		aerodynamics_,
		{ config_.aerodynamics, input },
		::Systems::make_aerodynamic_sinks(
			add_force_sink, add_moment_sink));
	apply_fallback_ground_forces();
	landing_gear_.update_on_ground();
	::Systems::AerodynamicsFrameInput shake_input = input;
	shake_input.on_ground = landing_gear_.data().on_ground;
	shake_input.g_force = aircraft_state_.g;
	gameplay_.shake_amplitude = ::Systems::update_aerodynamic_shake(
		aerodynamics_,
		config_.aerodynamics,
		shake_input);
}

void AircraftSimulation::apply_fallback_ground_forces()
{
	const ::Systems::EngineSystemState& engines = engine_.state();
	const ::Systems::LandingGearSystemState& gear =
		landing_gear_.device_state();
	const ::Systems::SuspensionFallbackInput input = {
		aircraft_state_.altitude_agl,
		aircraft_state_.pitch,
		aircraft_state_.roll,
		aircraft_state_.velocity_world.y,
		aircraft_state_.velocity_body.x,
		gear.position,
		aircraft_state_.current_mass,
		engines.left.throttle_input,
		engines.right.throttle_input,
		left_thrust_force_,
		right_thrust_force_,
		gear.wheels.brake_left,
		gear.wheels.brake_right
	};
	ground_physics_.fallback_ground_force =
		::Systems::apply_fallback_ground_forces(
			ground_physics_,
		::Systems::make_suspension_fallback_context(
			config_.suspension, input),
		[this](const Common::Vec3& force, const Common::Vec3& position)
		{
			add_force(force, position);
		});
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
	::Systems::mark_first_frame_completed(startup_);
}

void AircraftSimulation::repair(const RepairEvent& event)
{
	airframe_structure_.repair(event);
	engine_.repair(event);
}
}
}
