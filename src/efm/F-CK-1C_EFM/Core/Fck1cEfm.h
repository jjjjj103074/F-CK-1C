#pragma once

#include "AircraftState.h"
#include "../Common/Vec3.h"
#include "../Systems/AerodynamicsSystem.h"
#include "../Systems/AirframeDeviceSystem.h"
#include "../Systems/DamageModel.h"
#include "../Systems/EngineSystem.h"
#include "../Systems/FBWController.h"
#include "../Systems/FuelSystem.h"
#include "../Systems/InputSystem.h"
#include "../Systems/LandingGearSystem.h"
#include "../Systems/StartupSystem.h"
#include "../Systems/SuspensionSystem.h"

namespace Core
{
class Fck1cEfm;

struct AutopilotCommand
{
	bool master = false;
	bool bypass = false;
	bool auto_throttle_engaged = false;
	double pitch_command = 0.0;
	double roll_command = 0.0;
	double throttle_command = 0.0;
};

struct MaxPowerCommand
{
	double ready = 0.0;
	double value = 1.0;
};

class Fck1cEfmRuntime
{
public:
	virtual ~Fck1cEfmRuntime() = default;
	virtual AutopilotCommand read_autopilot() = 0;
	virtual MaxPowerCommand read_max_power() = 0;
	virtual void on_first_frame(const Fck1cEfm& efm) = 0;
	virtual void on_engine_shutdown(const Fck1cEfm& efm) = 0;
	virtual void on_thrust_updated(const Fck1cEfm& efm, const MaxPowerCommand& command) = 0;
	virtual void on_ground_diagnostics(const Fck1cEfm& efm, double dt) = 0;
};

struct EngineSimulationConfig
{
	double fuel_consumption = 0.0;
	double start_time = 0.0;
	double spool_up_tau = 0.0;
	double spool_down_tau = 0.0;
	const double* mach_table = nullptr;
	const double* max_thrust_table = nullptr;
	unsigned mach_table_size = 0;
	const double* throttle_input_table = nullptr;
	const double* power_table = nullptr;
	unsigned throttle_table_size = 0;
};

struct Fck1cEfmConfig
{
	Systems::AerodynamicsSystemConfig aerodynamics;
	Systems::SuspensionSystemConfig suspension;
	Systems::FBWControllerConfig fbw;
	EngineSimulationConfig engine;
	Common::Vec3 left_engine_position;
	Common::Vec3 right_engine_position;
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
	bool invincible = false;
	bool infinite_fuel = false;
	bool easy_flight = false;
	double shake_amplitude = 0.0;
};

struct Fck1cEfmSystems
{
	Systems::AerodynamicsSystemState aerodynamics;
	Systems::PrimaryControlState primary_controls;
	Systems::EngineSystemState engines;
	Systems::ThrottleInputState throttle_inputs;
	Systems::AirframeDeviceState airframe_devices;
	Systems::WheelState wheels;
	Systems::FuelSystem fuel;
	Systems::SuspensionSystemState suspension;
	Systems::DamageModel damage;
	Systems::StartupSystemState startup;
	Systems::FBWControllerState fbw;
};

class Fck1cEfm
{
public:
	Fck1cEfm(const Fck1cEfmConfig& config, Fck1cEfmRuntime& runtime);

	Fck1cEfm(const Fck1cEfm&) = delete;
	Fck1cEfm& operator=(const Fck1cEfm&) = delete;

	const Fck1cEfmConfig& config() const;
	AircraftState& aircraft_state();
	const AircraftState& aircraft_state() const;
	ForceMomentFrame& force_moment();
	const ForceMomentFrame& force_moment() const;
	ControlSurfaceState& control_surfaces();
	const ControlSurfaceState& control_surfaces() const;
	GameplayState& gameplay();
	const GameplayState& gameplay() const;
	Fck1cEfmSystems& systems();
	const Fck1cEfmSystems& systems() const;
	void simulate(double dt);

private:
	void begin_frame(double dt);
	void update_airframe(double dt);
	void update_autopilot();
	void update_fbw(double dt);
	Systems::FBWControllerInput make_fbw_input(double dt) const;
	Systems::AerodynamicsFrameInput make_aerodynamics_input() const;
	void update_primary_aerodynamics(const Systems::AerodynamicsFrameInput& input);
	void update_engines_and_fuel(double dt);
	double max_dry_thrust() const;
	void update_engine_state(double dt, double dry_thrust);
	void handle_engine_shutdown(double dt);
	void apply_thrust_and_observe();
	void update_fuel(double dt);
	void update_ground_and_suspension(double dt, const Systems::AerodynamicsFrameInput& input);
	void apply_fallback_ground_forces();
	double nose_wheel_steering() const;
	void add_force(const Common::Vec3& force, const Common::Vec3& position);
	void add_moment(const Common::Vec3& moment);
	void finish_frame();

	const Fck1cEfmConfig config_;
	Fck1cEfmRuntime& runtime_;
	AircraftState aircraft_state_;
	ForceMomentFrame force_moment_;
	ControlSurfaceState control_surfaces_;
	GameplayState gameplay_;
	Fck1cEfmSystems systems_;
};
}
