#pragma once

#include "AircraftState.h"
#include "FrameContracts.h"
#include "../Common/Vec3.h"
#include "../Data/AircraftConfig.h"
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

#include <cstddef>

namespace Core
{
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
	Systems::LandingGearSystemState landing_gear;
	Systems::FuelSystem fuel;
	Systems::SuspensionSystemState suspension;
	Systems::DamageModel damage;
	Systems::StartupSystemState startup;
	Systems::FBWControllerState fbw;
};

struct Fck1cEfmSnapshot
{
	AircraftState aircraft;
	ForceMomentFrame force_moment;
	ControlSurfaceState control_surfaces;
	GameplayState gameplay;
	Fck1cEfmSystems systems;
	bool suspension_feedback_available = false;
	bool any_weight_on_wheels = false;
};

enum class CommandGroup
{
	PitchRoll,
	Yaw,
	Fbw,
	Engine,
	Throttle,
	Airframe,
	LandingGear,
	None
};

enum class CommandAction
{
	NoOp,
	SetPitchAxis,
	SetPitchDiscrete,
	AdjustPitchTrim,
	SetRollAxis,
	SetRollDiscrete,
	AdjustRollTrim,
	SetYawAxis,
	SetYawDiscrete,
	AdjustYawTrim,
	ResetTrim,
	ToggleFbwCat,
	SetFbwCat1,
	SetFbwCat3,
	SetGLimiterOverride,
	ToggleGLimiterOverride,
	SetBothEngines,
	SetLeftEngine,
	SetRightEngine,
	SetCommonThrottleAxis,
	SetLeftThrottleAxis,
	SetRightThrottleAxis,
	StepCommonThrottle,
	StepLeftThrottle,
	StepRightThrottle,
	ToggleAirbrake,
	SetAirbrake,
	ToggleFlaps,
	SetFlapsUp,
	SetFlapsAuto,
	SetFlapsDown,
	ToggleGear,
	SetGear,
	ToggleNoseWheelSteering,
	SetNoseWheelSteering,
	SetBrake,
	SetLeftBrake,
	SetRightBrake
};

struct EfmCommand
{
	CommandGroup group = CommandGroup::None;
	CommandAction action = CommandAction::NoOp;
	double value = 0.0;
};

struct ExternalFuelInput
{
	int station = 0;
	double fuel = 0.0;
	Common::Vec3 position;
};

enum class DamageArea
{
	LeftWing,
	RightWing,
	Tail,
	LeftEngine,
	RightEngine
};

struct DamageEvent
{
	DamageArea area = DamageArea::LeftWing;
	std::size_t segment = 0;
	double integrity = 1.0;
};

struct MassDelta
{
	double mass = 0.0;
	Common::Vec3 position;
	Common::Vec3 moment_of_inertia;
};

struct MassDeltaResult
{
	bool available = false;
	MassDelta delta;
};

class Fck1cEfmRuntime
{
public:
	virtual ~Fck1cEfmRuntime() = default;
};

class Fck1cEfm
{
public:
	Fck1cEfm(const Data::AircraftConfig& config, Fck1cEfmRuntime& runtime);
	Fck1cEfm(const Fck1cEfm&) = delete;
	Fck1cEfm& operator=(const Fck1cEfm&) = delete;

	const Data::AircraftConfig& config() const;
	Fck1cEfmSnapshot snapshot() const;
	FrameOutput frame_output(const FrameDataAvailability& availability) const;
	ForceMomentFrame force_moment_output() const;
	double max_dry_thrust_at(double mach) const;
	double internal_fuel() const;
	double external_fuel() const;
	double shake_amplitude() const;

	void set_atmosphere(const AtmosphereInput& input);
	void set_surface(const SurfaceInput& input);
	void set_mass_state(const MassStateInput& input);
	void set_world_kinematics(const WorldKinematicsInput& input);
	void set_body_kinematics(const BodyKinematicsInput& input);
	void handle_command(const EfmCommand& command);
	MassDeltaResult take_mass_delta();
	void set_internal_fuel(double fuel);
	void set_external_fuel(const ExternalFuelInput& input);
	void add_refueling_fuel(double fuel);
	void set_infinite_fuel(bool enabled);
	void set_easy_flight(bool enabled);
	void set_invincible(bool enabled);
	void apply_damage(const DamageEvent& event);
	bool update_suspension_feedback(const SuspensionFeedbackInput& input);

	void simulate(
		double dt,
		const AutopilotCommand& autopilot,
		const MaxPowerCommand& max_power);
	void cold_start();
	void hot_ground_start();
	void hot_air_start();
	void release();
	void repair();

private:
	void handle_pitch_roll_command(const EfmCommand& command);
	void handle_yaw_command(const EfmCommand& command);
	void handle_fbw_command(const EfmCommand& command);
	void handle_engine_command(const EfmCommand& command);
	void handle_throttle_command(const EfmCommand& command);
	void handle_airframe_command(const EfmCommand& command);
	void handle_landing_gear_command(const EfmCommand& command);
	void begin_frame(double dt);
	void update_airframe(double dt);
	void update_autopilot(const AutopilotCommand& command);
	void update_fbw(double dt);
	Systems::FBWControllerInput make_fbw_input(double dt) const;
	Systems::AerodynamicsFrameInput make_aerodynamics_input() const;
	void update_primary_aerodynamics(const Systems::AerodynamicsFrameInput& input);
	void update_engines_and_fuel(double dt, const MaxPowerCommand& max_power);
	double max_dry_thrust() const;
	void update_engine_state(double dt, double dry_thrust);
	void handle_engine_shutdown(double dt);
	void apply_thrust(const MaxPowerCommand& command);
	void update_fuel(double dt);
	void update_ground_and_suspension(double dt, const Systems::AerodynamicsFrameInput& input);
	void apply_fallback_ground_forces();
	double nose_wheel_steering() const;
	void add_force(const Common::Vec3& force, const Common::Vec3& position);
	void add_moment(const Common::Vec3& moment);
	void finish_frame();
	void reset_start_state(Systems::StartupMode mode);
	void reset_control_outputs();

	const Data::AircraftConfig config_;
	Fck1cEfmRuntime& runtime_;
	AircraftState aircraft_state_;
	ForceMomentFrame force_moment_;
	ControlSurfaceState control_surfaces_;
	GameplayState gameplay_;
	Fck1cEfmSystems systems_;
};
}
