#include "AircraftSimulation.h"

#include <stdexcept>

namespace
{
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
	if (!Data::has_valid_aerodynamics_definition(
		config.aerodynamics))
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
	system_pipeline_(make_system_setup(config, setup)),
	simulation_pipeline_(config)
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
	return make_frame_output(
		system_pipeline_.snapshot(),
		simulation_pipeline_.result(),
		{});
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
		system_pipeline_.suppress_fuel_consumption();
	}
	const Systems::AircraftDataSnapshot aircraft =
		system_pipeline_.step(input, options);
	const SimulationResult& simulation = simulation_pipeline_.step({
		aircraft,
		aircraft_state_,
		input,
		gameplay_.options.easy_flight
	});
	finish_frame();
	return make_frame_output(
		aircraft,
		simulation,
		input.availability);
}

void AircraftSimulation::begin_frame(double dt)
{
	::Systems::advance_simulation_time(startup_, dt);
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
