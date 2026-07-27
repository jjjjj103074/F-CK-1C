#pragma once

#include "../AircraftState.h"
#include "../Contracts/Commands.h"
#include "../Contracts/Events.h"
#include "../Contracts/FrameContracts.h"
#include "../Systems/SystemPipeline.h"
#include "../../Common/Vec3.h"
#include "../../Data/AircraftConfig.h"
#include "../../Systems/AerodynamicsSystem.h"
#include "../../Systems/StartupSystem.h"

#include <map>

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

struct ForceMomentFrame
{
	Common::Vec3 force;
	Common::Vec3 moment;
	Common::Vec3 center_of_mass;
};

struct GameplayState
{
	SimulationOptions options;
	double shake_amplitude = 0.0;
};

class AircraftSimulation final
{
public:
	AircraftSimulation(
		const Data::AircraftConfig& config,
		const FlightSetupContext& setup);

	FrameOutput initial_output() const;
	FrameOutput step(const FrameInput& input);
	void handle_command(const Command& command);
	double internal_fuel() const;
	double external_fuel() const;
	FlightFuelLoad fuel_load() const;
	MassDeltaResult take_mass_delta();
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
		const FrameDataAvailability& availability) const;
	void apply_setup(const FlightSetupContext& setup);
	void apply_frame_input(const FrameInput& input);
	void begin_frame(double dt);
	void update_airframe(const Systems::AircraftDataSnapshot& aircraft);
	::Systems::AerodynamicsFrameInput make_aerodynamics_input(
		const Systems::AircraftDataSnapshot& aircraft) const;
	void update_primary_aerodynamics(
		const ::Systems::AerodynamicsFrameInput& input);
	void update_engines(
		const Systems::AircraftDataSnapshot& aircraft,
		const MaxPowerCommand& max_power);
	double max_dry_thrust() const;
	void update_engine_thrust(
		const EngineData& engines,
		double dry_thrust);
	void apply_thrust(const MaxPowerCommand& command);
	void update_ground_and_suspension(
		const Systems::AircraftDataSnapshot& aircraft,
		const ::Systems::AerodynamicsFrameInput& input);
	void apply_fallback_ground_forces(
		const Systems::AircraftDataSnapshot& aircraft);
	void add_force(
		const Common::Vec3& force,
		const Common::Vec3& position);
	void add_moment(const Common::Vec3& moment);
	void finish_frame();

	const Data::AircraftConfig& config_;
	AircraftState aircraft_state_;
	ForceMomentFrame force_moment_;
	GameplayState gameplay_;
	Systems::SystemPipeline system_pipeline_;
	::Systems::AerodynamicsSystemState aerodynamics_;
	::Systems::StartupSystemState startup_;
	::Systems::SuspensionSystemState ground_physics_;
	double left_thrust_force_ = 0.0;
	double right_thrust_force_ = 0.0;
};

void validate_aircraft_config(const Data::AircraftConfig& config);
}
}
