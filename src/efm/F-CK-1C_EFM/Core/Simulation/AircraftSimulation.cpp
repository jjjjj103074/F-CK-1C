#include "AircraftSimulation.h"

#include "Models/Aerodynamics/AerodynamicsConfig.h"
#include "Models/Aerodynamics/AerodynamicsModel.h"
#include "Models/GroundInteraction/GroundInteractionConfig.h"
#include "Models/GroundInteraction/GroundInteractionModel.h"
#include "Models/MassProperties/MassPropertiesModel.h"
#include "Models/Propulsion/PropulsionConfig.h"
#include "Models/Propulsion/PropulsionModel.h"

#include <utility>

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
	const Core::Simulation::FlightSetupContext& setup)
{
	return {
		setup.start_mode,
		make_system_fuel_state(setup.fuel)
	};
}

Core::Simulation::SimulationModels make_fck1c_simulation_models()
{
	Core::Simulation::SimulationModels models;
	models.aerodynamics =
		std::make_unique<Core::Simulation::AerodynamicsModel>(
			Core::Simulation::fck1c_aerodynamics_config());
	models.propulsion =
		std::make_unique<Core::Simulation::PropulsionModel>(
			Core::Simulation::fck1c_propulsion_config());
	models.ground_interaction =
		std::make_unique<Core::Simulation::GroundInteractionModel>(
			Core::Simulation::fck1c_ground_interaction_config());
	models.mass_properties =
		std::make_unique<Core::Simulation::MassPropertiesModel>();
	return models;
}
}

namespace Core
{
namespace Simulation
{
AircraftSimulation::AircraftSimulation(
	const FlightSetupContext& setup,
	AircraftSimulationDependencies dependencies)
	: system_pipeline_(
		make_system_setup(setup),
		std::move(dependencies.system_catalog)),
	simulation_pipeline_(std::move(dependencies.simulation_models))
{
	apply_setup(setup);
}

void AircraftSimulation::apply_setup(const FlightSetupContext& setup)
{
	gameplay_.options = setup.options;
	simulation_time_s_ = 0.0;
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
	const AircraftObservation observation =
		make_aircraft_observation(aircraft_state_);
	const Systems::AircraftDataSnapshot aircraft =
		system_pipeline_.step({ input, observation }, options);
	const SimulationResult& simulation = simulation_pipeline_.step({
		aircraft,
		aircraft_state_,
		input,
		gameplay_.options.easy_flight
	});
	return make_frame_output(
		aircraft,
		simulation,
		input.availability);
}

void AircraftSimulation::begin_frame(double dt)
{
	simulation_time_s_ += dt;
}

void AircraftSimulation::repair(const RepairEvent& event)
{
	(void)system_pipeline_.apply(event);
}

AircraftSimulationFactory make_fck1c_aircraft_simulation_factory()
{
	return [](const FlightSetupContext& setup)
	{
		AircraftSimulationDependencies dependencies;
		dependencies.system_catalog =
			Systems::load_generated_system_catalog();
		dependencies.simulation_models =
			make_fck1c_simulation_models();
		return std::make_unique<AircraftSimulation>(
			setup,
			std::move(dependencies));
	};
}

double carrier_launch_reference_thrust()
{
	return fck1c_carrier_launch_reference_thrust();
}
}
}
