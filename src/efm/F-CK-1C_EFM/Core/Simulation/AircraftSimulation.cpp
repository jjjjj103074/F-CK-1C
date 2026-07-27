#include "AircraftSimulation.h"

#include "../ForceMoment.h"

#include <stdexcept>

namespace
{
constexpr double kMaxPowerReadyThreshold = 0.5;
constexpr double kMaxPowerCutThreshold = 0.5;

Core::Systems::FlightFuelState make_system_fuel_state(
	const Core::Simulation::FlightFuelLoad& load)
{
	Core::Systems::FlightFuelState result;
	result.internal_fuel = load.internal_fuel;
	result.external_fuel.reserve(load.external_fuel_by_station.size());
	for (const auto& station : load.external_fuel_by_station)
	{
		result.external_fuel.push_back({
			station.first,
			station.second.fuel,
			station.second.position
		});
	}
	return result;
}

Core::Systems::FlightSetupContext make_system_setup(
	const Data::AircraftConfig& config,
	const Core::Simulation::FlightSetupContext& setup)
{
	return {
		config,
		setup.start_mode,
		make_system_fuel_state(setup.fuel)
	};
}

::Systems::EngineSystemState make_engine_physics_state(
	const Core::EngineData& source)
{
	::Systems::EngineSystemState result;
	result.left.throttle_output = source.left.throttle_output;
	result.left.afterburner_ratio = source.left.afterburner_ratio;
	result.right.throttle_output = source.right.throttle_output;
	result.right.afterburner_ratio = source.right.afterburner_ratio;
	return result;
}

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
	system_pipeline_(make_system_setup(config, setup))
{
	validate_aircraft_config(config_);
	apply_setup(setup);
}

void AircraftSimulation::apply_setup(const FlightSetupContext& setup)
{
	gameplay_.options = setup.options;
	::Systems::begin_startup(startup_, startup_mode(setup.start_mode));
}

FrameOutput AircraftSimulation::initial_output() const
{
	return make_frame_output(system_pipeline_.snapshot(), {});
}

double AircraftSimulation::internal_fuel() const
{
	return system_pipeline_.snapshot()
		.read(AircraftDataKeys::kFuelData).internal_fuel;
}

double AircraftSimulation::external_fuel() const
{
	return system_pipeline_.snapshot()
		.read(AircraftDataKeys::kFuelData).external_fuel;
}

FlightFuelLoad AircraftSimulation::fuel_load() const
{
	const Systems::FlightFuelState state = system_pipeline_.fuel_state();
	FlightFuelLoad load;
	load.internal_fuel = state.internal_fuel;
	for (const ExternalFuelInput& external : state.external_fuel)
	{
		load.external_fuel_by_station[external.station] = {
			external.fuel,
			external.position
		};
	}
	return load;
}

MassDeltaResult AircraftSimulation::take_mass_delta()
{
	return system_pipeline_.take_mass_delta();
}

void AircraftSimulation::set_internal_fuel(double fuel)
{
	system_pipeline_.set_internal_fuel(fuel);
}

void AircraftSimulation::set_external_fuel(
	int station,
	const ExternalFuelLoad& fuel)
{
	system_pipeline_.set_external_fuel({
		station,
		fuel.fuel,
		fuel.position
	});
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
	(void)system_pipeline_.apply(event);
	return { false };
}

void AircraftSimulation::apply_frame_input(const FrameInput& input)
{
	Core::apply_aircraft_observations(aircraft_state_, input);
	if (input.availability.mass)
	{
		force_moment_.center_of_mass = input.mass.center_of_mass;
	}
}

FrameOutput AircraftSimulation::step(const FrameInput& input)
{
	apply_frame_input(input);
	begin_frame(input.dt_s);
	update_airspeed(aircraft_state_);
	const Systems::SystemStepOptions options = {
		!gameplay_.options.infinite_fuel
	};
	if (!options.advance_fuel)
	{
		system_pipeline_.set_reported_fuel_flow(0.0);
	}
	const Systems::AircraftDataSnapshot aircraft =
		system_pipeline_.step(input, options);
	update_airframe(aircraft);
	const ::Systems::AerodynamicsFrameInput aerodynamics_input =
		make_aerodynamics_input(aircraft);
	update_primary_aerodynamics(aerodynamics_input);
	update_engines(aircraft, input.max_power);
	update_ground_and_suspension(aircraft, aerodynamics_input);
	finish_frame();
	return make_frame_output(aircraft, input.availability);
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

void AircraftSimulation::update_airframe(
	const Systems::AircraftDataSnapshot& aircraft)
{
	const SecondaryControlPosition& secondary =
		aircraft.read(AircraftDataKeys::kSecondaryControlPosition);
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
			secondary.slats
		});
}

::Systems::AerodynamicsFrameInput
	AircraftSimulation::make_aerodynamics_input(
		const Systems::AircraftDataSnapshot& aircraft) const
{
	const PrimaryControlPosition& primary =
		aircraft.read(AircraftDataKeys::kPrimaryControlPosition);
	const SecondaryControlPosition& secondary =
		aircraft.read(AircraftDataKeys::kSecondaryControlPosition);
	const LandingGearData& gear =
		aircraft.read(AircraftDataKeys::kLandingGearData);
	const AirframeIntegrity& integrity =
		aircraft.read(AircraftDataKeys::kAirframeIntegrity);
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
	input.elevator_command = primary.elevator;
	input.aileron_command = primary.aileron;
	input.rudder_command = primary.rudder;
	input.airbrake_pos = secondary.airbrake;
	input.flaps_pos = secondary.flaps;
	input.gear_pos = gear.position;
	input.left_wing_integrity = integrity.left_wing;
	input.right_wing_integrity = integrity.right_wing;
	input.tail_integrity = integrity.tail;
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

void AircraftSimulation::update_engines(
	const Systems::AircraftDataSnapshot& aircraft,
	const MaxPowerCommand& max_power)
{
	const double dry_thrust = max_dry_thrust();
	update_engine_thrust(
		aircraft.read(AircraftDataKeys::kEngineData),
		dry_thrust);
	apply_thrust(max_power);
}

double AircraftSimulation::max_dry_thrust() const
{
	return ::Systems::max_dry_thrust(
		config_.engine,
		aircraft_state_.mach);
}

void AircraftSimulation::update_engine_thrust(
	const EngineData& engines,
	double dry_thrust)
{
	::Systems::EngineSystemState physical_state =
		make_engine_physics_state(engines);
	::Systems::update_engine_thrust_outputs(
		physical_state,
		config_.engine,
		{
			dry_thrust,
			aircraft_state_.engine_alt_effect,
			engines.left.condition,
			engines.right.condition
		});
	left_thrust_force_ = physical_state.left.thrust_force;
	right_thrust_force_ = physical_state.right.thrust_force;
	if (engines.thrust_inhibited)
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

void AircraftSimulation::update_ground_and_suspension(
	const Systems::AircraftDataSnapshot& aircraft,
	const ::Systems::AerodynamicsFrameInput& input)
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
	::Systems::apply_aerodynamic_limiters(
		aerodynamics_,
		{ config_.aerodynamics, input },
		::Systems::make_aerodynamic_sinks(
			add_force_sink, add_moment_sink));
	apply_fallback_ground_forces(aircraft);
	::Systems::AerodynamicsFrameInput shake_input = input;
	shake_input.on_ground =
		aircraft.read(AircraftDataKeys::kLandingGearData).on_ground;
	shake_input.g_force = aircraft_state_.g;
	gameplay_.shake_amplitude = ::Systems::update_aerodynamic_shake(
		aerodynamics_,
		config_.aerodynamics,
		shake_input);
}

void AircraftSimulation::apply_fallback_ground_forces(
	const Systems::AircraftDataSnapshot& aircraft)
{
	const EngineData& engines =
		aircraft.read(AircraftDataKeys::kEngineData);
	const LandingGearData& gear =
		aircraft.read(AircraftDataKeys::kLandingGearData);
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
		gear.brake_left,
		gear.brake_right
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
	(void)system_pipeline_.apply(event);
}
}
}
