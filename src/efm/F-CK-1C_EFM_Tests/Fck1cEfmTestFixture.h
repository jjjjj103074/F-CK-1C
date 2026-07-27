#pragma once

#include "Core/Fck1cEfm.h"
#include "Core/Simulation/AircraftSimulation.h"
#include "Core/Simulation/Models/Aerodynamics/AerodynamicsConfig.h"
#include "Core/Simulation/Models/Aerodynamics/AerodynamicsModel.h"
#include "Core/Simulation/Models/GroundInteraction/GroundInteractionConfig.h"
#include "Core/Simulation/Models/GroundInteraction/GroundInteractionModel.h"
#include "Core/Simulation/Models/MassProperties/MassPropertiesModel.h"
#include "Core/Simulation/Models/Propulsion/PropulsionConfig.h"
#include "Core/Simulation/Models/Propulsion/PropulsionModel.h"
#include "Core/Systems/Engine/Engine.h"
#include "Core/Systems/FlightControlComputer/FlightControlComputer.h"
#include "Core/Systems/LandingGear/LandingGear.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace Tests
{
namespace Fck1c
{
inline constexpr double kTestAfterburnerThrustFactor = 1.73;

struct TestAircraftConfig;

Core::Simulation::AircraftSimulationFactory make_test_simulation_factory(
	const TestAircraftConfig& config);

struct TestAircraftConfig
{
	Core::Simulation::AerodynamicsConfig aerodynamics;
	Core::Systems::EngineConfig engine;
	Core::Systems::FlightControlComputerConfig flight_control_computer;
	Core::Systems::LandingGearConfig landing_gear;
	Core::Simulation::PropulsionConfig propulsion;
	Core::Simulation::GroundInteractionConfig ground_interaction;

	operator Core::Simulation::AircraftSimulationFactory() const
	{
		return make_test_simulation_factory(*this);
	}
};

inline TestAircraftConfig make_test_config()
{
	TestAircraftConfig config;
	config.aerodynamics.wing_area = 24.26;
	config.aerodynamics.wingspan = 8.53;
	config.aerodynamics.length = 14.48;
	config.aerodynamics.height = 4.7;
	config.aerodynamics.mach_max = 1.5;
	config.aerodynamics.mach_table = { 0.0, 1.0 };
	config.aerodynamics.cx_zero_table = { 0.025, 0.030 };
	config.aerodynamics.cy_alpha_table = { 0.05, 0.04 };
	config.aerodynamics.roll_rate_max_table = { 3.0, 2.0 };
	config.aerodynamics.alpha_max_table = { 20.0, 18.0 };
	config.aerodynamics.cy_max_table = { 1.2, 1.0 };
	config.flight_control_computer.mach_table =
		config.aerodynamics.mach_table;
	config.flight_control_computer.alpha_limit_deg =
		config.aerodynamics.alpha_max_table;
	config.engine.start_time = 5.0;
	config.engine.spool_up_tau = 1.0;
	config.engine.spool_down_tau = 1.0;
	config.engine.throttle_input_table = { 0.0, 1.0 };
	config.engine.power_table = { 0.1, 1.0 };
	config.propulsion.mach_table = { 0.0, 1.0 };
	config.propulsion.max_thrust_table = { 54000.0, 50000.0 };
	config.propulsion.afterburner_thrust_factor =
		kTestAfterburnerThrustFactor;
	config.propulsion.left_engine_position = { -3.793, -0.391, -0.716 };
	config.propulsion.right_engine_position = { -3.793, -0.391, 0.716 };
	config.landing_gear =
		Core::Systems::fck1c_landing_gear_config();
	config.ground_interaction =
		Core::Simulation::fck1c_ground_interaction_config();
	return config;
}

inline void replace_system_entry(
	std::vector<Core::Systems::SystemEntry>& catalog,
	Core::Systems::SystemEntry replacement)
{
	const auto entry = std::find_if(
		catalog.begin(),
		catalog.end(),
		[&replacement](const Core::Systems::SystemEntry& candidate)
		{
			return candidate.id == replacement.id;
		});
	if (entry == catalog.end())
	{
		throw std::logic_error(
			"Test catalog is missing System '" + replacement.id + "'.");
	}
	*entry = std::move(replacement);
}

inline Core::Simulation::SimulationModels make_test_models(
	const TestAircraftConfig& config)
{
	Core::Simulation::SimulationModels models;
	models.aerodynamics =
		std::make_unique<Core::Simulation::AerodynamicsModel>(
			config.aerodynamics);
	models.propulsion =
		std::make_unique<Core::Simulation::PropulsionModel>(
			config.propulsion);
	models.ground_interaction =
		std::make_unique<Core::Simulation::GroundInteractionModel>(
			config.ground_interaction);
	models.mass_properties =
		std::make_unique<Core::Simulation::MassPropertiesModel>();
	return models;
}

inline Core::Simulation::AircraftSimulationDependencies
	make_test_dependencies(const TestAircraftConfig& config)
{
	Core::Simulation::AircraftSimulationDependencies dependencies;
	dependencies.system_catalog =
		Core::Systems::load_generated_system_catalog();
	replace_system_entry(
		dependencies.system_catalog,
		Core::Systems::make_engine_system_entry(config.engine));
	replace_system_entry(
		dependencies.system_catalog,
		Core::Systems::make_flight_control_computer_system_entry(
			config.flight_control_computer));
	replace_system_entry(
		dependencies.system_catalog,
		Core::Systems::make_landing_gear_system_entry(
			config.landing_gear));
	dependencies.simulation_models = make_test_models(config);
	return dependencies;
}

inline Core::Simulation::AircraftSimulationFactory
	make_test_simulation_factory(const TestAircraftConfig& config)
{
	return [owned_config = config](
		const Core::Simulation::FlightSetupContext& setup)
	{
		return std::make_unique<Core::Simulation::AircraftSimulation>(
			setup,
			make_test_dependencies(owned_config));
	};
}

inline Core::FrameDataAvailability all_frame_data_available()
{
	return { true, true, true, true, true, { true, true, true } };
}

inline Core::FrameInput make_frame_input()
{
	Core::FrameInput input;
	input.dt_s = 0.02;
	input.availability = all_frame_data_available();
	input.atmosphere = {
		1200.0, 281.0, 330.0, 1.1, 88000.0, { 5.0, 1.0, -2.0 }
	};
	input.surface = { 200.0, 203.0, 4, { 0.0, 1.0, 0.0 } };
	input.mass = { 9400.0, { 0.2, -0.1, 0.3 }, { 11.0, 12.0, 13.0 } };
	input.world_kinematics = {
		{ 0.1, 0.2, 0.3 }, { 150.0, 4.0, 2.0 }, { 10.0, 20.0, 1200.0 },
		{ 0.01, 0.02, 0.03 }, { 0.1, 0.2, 0.3 }, { 0.0, 0.0, 0.0, 1.0 }
	};
	input.body_kinematics = {
		{ 0.0, 9.81, 0.0 }, { 140.0, 3.0, 1.0 }, { 4.0, 0.5, -1.0 },
		{ 0.02, 0.03, 0.04 }, { 0.05, 0.06, 0.07 },
		0.3, 0.1, -0.2, 0.15, -0.04
	};
	input.suspension = {
		Core::SuspensionFeedbackInput{
			0, { 3.0, 4.0, 0.0 }, { 1.0, 2.0, 3.0 }, 0.9, 0.10, 12.0 },
		Core::SuspensionFeedbackInput{
			1, { 0.0, 80.0, 0.0 }, { 4.0, 5.0, 6.0 }, 0.8, 0.20, 13.0 },
		Core::SuspensionFeedbackInput{
			2, { 0.0, 90.0, 0.0 }, { 7.0, 8.0, 9.0 }, 0.7, 0.30, 14.0 }
	};
	input.autopilot = { true, false, true, 0.2, -0.3, 0.4 };
	input.max_power = { 1.0, 1.0 };
	return input;
}
}
}
