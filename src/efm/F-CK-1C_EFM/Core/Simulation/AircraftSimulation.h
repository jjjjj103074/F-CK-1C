#pragma once

#include "../AircraftState.h"
#include "../Contracts/Commands.h"
#include "../Contracts/Events.h"
#include "../Contracts/FrameContracts.h"
#include "../../Common/Vec3.h"
#include "../../Data/AircraftConfig.h"
#include "../../Systems/AerodynamicsSystem.h"
#include "../../Systems/AirframeDeviceSystem.h"
#include "../../Systems/DamageModel.h"
#include "../../Systems/EngineSystem.h"
#include "../../Systems/FBWController.h"
#include "../../Systems/FuelSystem.h"
#include "../../Systems/InputSystem.h"
#include "../../Systems/LandingGearSystem.h"
#include "../../Systems/StartupSystem.h"
#include "../../Systems/SuspensionSystem.h"

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

struct ControlSurfaceState
{
	double elevator_command = 0.0;
	double aileron_command = 0.0;
	double rudder_command = 0.0;
};

struct GameplayState
{
	SimulationOptions options;
	double shake_amplitude = 0.0;
};

struct AircraftSystems
{
	Systems::AerodynamicsSystemState aerodynamics;
	Systems::PrimaryControlState primary_controls;
	Systems::EngineSystemState engines;
	Systems::ThrottleInputState throttle_inputs;
	Systems::AirframeDeviceState airframe_devices;
	Systems::LandingGearSystemState landing_gear;
	Systems::FuelSystem fuel;
	Systems::SuspensionSystemState suspension;
	Systems::DamageModel damage;
	Systems::StartupSystemState startup;
	Systems::FBWControllerState fbw;
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
		const FrameDataAvailability& availability) const;
	void apply_setup(const FlightSetupContext& setup);
	void apply_frame_input(const FrameInput& input);
	void apply_mass_input(const MassStateInput& input);
	void apply_suspension_input(const FrameInput& input);
	void configure_start_state(const FlightSetupContext& setup);
	void handle_pitch_roll_command(const Command& command);
	void handle_yaw_command(const Command& command);
	void handle_fbw_command(const Command& command);
	void handle_engine_command(const Command& command);
	void handle_throttle_command(const Command& command);
	void handle_airframe_command(const Command& command);
	void handle_landing_gear_command(const Command& command);
	void begin_frame(double dt);
	void update_airframe(double dt);
	void update_autopilot(const AutopilotCommand& command);
	void update_fbw(double dt);
	Systems::FBWControllerInput make_fbw_input(double dt) const;
	Systems::AerodynamicsFrameInput make_aerodynamics_input() const;
	void update_primary_aerodynamics(
		const Systems::AerodynamicsFrameInput& input);
	void update_engines_and_fuel(
		double dt,
		const MaxPowerCommand& max_power);
	double max_dry_thrust() const;
	void update_engine_state(double dt, double dry_thrust);
	void handle_engine_shutdown(double dt);
	void apply_thrust(const MaxPowerCommand& command);
	void update_fuel(double dt);
	void update_ground_and_suspension(
		double dt,
		const Systems::AerodynamicsFrameInput& input);
	void apply_fallback_ground_forces();
	double nose_wheel_steering() const;
	void add_force(
		const Common::Vec3& force,
		const Common::Vec3& position);
	void add_moment(const Common::Vec3& moment);
	void finish_frame();

	const Data::AircraftConfig& config_;
	AircraftState aircraft_state_;
	ForceMomentFrame force_moment_;
	ControlSurfaceState control_surfaces_;
	GameplayState gameplay_;
	AircraftSystems systems_;
};

void validate_aircraft_config(const Data::AircraftConfig& config);
}
}
