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
struct Fck1cEfmConfig
{
	Systems::AerodynamicsSystemConfig aerodynamics;
	Systems::SuspensionSystemConfig suspension;
	Systems::FBWControllerConfig fbw;
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
	explicit Fck1cEfm(const Fck1cEfmConfig& config);

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

private:
	const Fck1cEfmConfig config_;
	AircraftState aircraft_state_;
	ForceMomentFrame force_moment_;
	ControlSurfaceState control_surfaces_;
	GameplayState gameplay_;
	Fck1cEfmSystems systems_;
};
}
