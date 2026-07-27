#pragma once

#include "AircraftState.h"
#include "../Contracts/Commands.h"
#include "../Contracts/Events.h"
#include "../Contracts/FrameContracts.h"
#include "../Systems/SystemPipeline.h"
#include "AircraftSimulationFactory.h"
#include "SimulationPipeline.h"
#include "../../Common/Vec3.h"

#include <map>
#include <memory>
#include <vector>

namespace Core
{
namespace Simulation
{
struct ExternalFuelLoad
{
	double fuel = 0.0;
	Common::Vec3 position;
};

struct FlightFuelLoad
{
	double internal_fuel = 0.0;
	std::map<int, ExternalFuelLoad> external_fuel_by_station;
};

struct SimulationOptions
{
	bool invincible = false;
	bool infinite_fuel = false;
	bool easy_flight = false;
};

struct FlightSetupContext
{
	const StartMode start_mode = StartMode::ColdGround;
	const FlightFuelLoad fuel;
	const SimulationOptions options;
};

struct GameplayState
{
	SimulationOptions options;
};

struct AircraftSimulationDependencies
{
	std::vector<Systems::SystemEntry> system_catalog;
	SimulationModels simulation_models;
};

class AircraftSimulation final
{
public:
	AircraftSimulation(
		const FlightSetupContext& setup,
		AircraftSimulationDependencies dependencies);

	FrameOutput initial_output() const;
	FrameOutput step(const FrameInput& input);
	void handle_command(const Command& command);
	double internal_fuel() const;
	double external_fuel() const;
	FlightFuelLoad fuel_load() const;
	void set_internal_fuel(double fuel);
	void set_external_fuel(int station, const ExternalFuelLoad& fuel);
	void set_infinite_fuel(bool enabled);
	void set_easy_flight(bool enabled);
	void set_invincible(bool enabled);
	DamageApplyResult apply_damage(const DamageEvent& event);
	void repair(const RepairEvent& event);

private:
	FrameOutput make_frame_output(
		const Systems::AircraftDataSnapshot& aircraft,
		const SimulationResult& simulation,
		const FrameDataAvailability& availability) const;
	void apply_setup(const FlightSetupContext& setup);
	void apply_frame_input(const FrameInput& input);
	void begin_frame(double dt);

	AircraftState aircraft_state_;
	GameplayState gameplay_;
	Systems::SystemPipeline system_pipeline_;
	SimulationPipeline simulation_pipeline_;
	double simulation_time_s_ = 0.0;
};

AircraftSimulationFactory make_fck1c_aircraft_simulation_factory();
double carrier_launch_reference_thrust();
}
}
