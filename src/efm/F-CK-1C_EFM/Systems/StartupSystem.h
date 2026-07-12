#pragma once

#include "AirframeDeviceSystem.h"
#include "DamageModel.h"
#include "EngineSystem.h"
#include "FBWController.h"
#include "InputSystem.h"
#include "LandingGearSystem.h"
#include "SuspensionSystem.h"

namespace Systems
{
enum StartupMode
{
	STARTUP_MODE_RELEASED = 0,
	STARTUP_MODE_COLD_GROUND = 1,
	STARTUP_MODE_HOT_GROUND = 2,
	STARTUP_MODE_HOT_AIR = 3
};

struct StartupSystemState
{
	StartupMode mode = STARTUP_MODE_RELEASED;
	double simulation_time = 0.0;
	bool first_frame_completed = false;
};

inline void advance_simulation_time(StartupSystemState& state, double dt)
{
	state.simulation_time += dt;
}

inline void mark_first_frame_completed(StartupSystemState& state)
{
	state.first_frame_completed = true;
}

inline void reset_start_common(
	StartupSystemState& startup,
	StartupMode mode,
	DamageModel& damage,
	SuspensionSystemState& suspension,
	FBWControllerState& fbw,
	WheelState& wheels,
	double roll,
	double pitch,
	double alpha,
	double g)
{
	reset_damage_model(damage);
	reset_suspension_feedback_state(suspension);
	reset_fbw_state(fbw, roll, pitch, alpha, g);
	reset_wheel_brakes(wheels);
	reset_wheel_spin(wheels);
	suspension.on_ground = false;
	startup.mode = mode;
}

inline void configure_cold_ground_start(
	StartupSystemState& startup,
	DamageModel& damage,
	SuspensionSystemState& suspension,
	FBWControllerState& fbw,
	WheelState& wheels,
	AirframeDeviceState& devices,
	ThrottleInputState& throttles,
	EngineSystemState& engines,
	double roll,
	double pitch,
	double alpha,
	double g)
{
	reset_start_common(
		startup,
		STARTUP_MODE_COLD_GROUND,
		damage,
		suspension,
		fbw,
		wheels,
		roll,
		pitch,
		alpha,
		g);
	configure_ground_start_devices(devices);
	set_nose_turn_enabled(wheels, true);
	reset_throttle_inputs(throttles, 0.0, 0.0);
	configure_cold_start_engines(engines);
}

inline void configure_hot_ground_start(
	StartupSystemState& startup,
	DamageModel& damage,
	SuspensionSystemState& suspension,
	FBWControllerState& fbw,
	WheelState& wheels,
	AirframeDeviceState& devices,
	ThrottleInputState& throttles,
	EngineSystemState& engines,
	int flap_down_mode,
	double roll,
	double pitch,
	double alpha,
	double g)
{
	reset_start_common(
		startup,
		STARTUP_MODE_HOT_GROUND,
		damage,
		suspension,
		fbw,
		wheels,
		roll,
		pitch,
		alpha,
		g);
	configure_hot_ground_start_devices(devices, flap_down_mode);
	set_nose_turn_enabled(wheels, true);
	reset_throttle_inputs(throttles, 0.0, 0.0);
	configure_hot_ground_start_engines(engines);
}

inline void configure_hot_air_start(
	StartupSystemState& startup,
	DamageModel& damage,
	SuspensionSystemState& suspension,
	FBWControllerState& fbw,
	WheelState& wheels,
	AirframeDeviceState& devices,
	ThrottleInputState& throttles,
	EngineSystemState& engines,
	double roll,
	double pitch,
	double alpha,
	double g)
{
	reset_start_common(
		startup,
		STARTUP_MODE_HOT_AIR,
		damage,
		suspension,
		fbw,
		wheels,
		roll,
		pitch,
		alpha,
		g);
	configure_air_start_devices(devices);
	set_nose_turn_enabled(wheels, false);
	reset_throttle_inputs(throttles, 0.5, 0.5);
	configure_hot_air_start_engines(engines);
}

inline void reset_primary_commands_for_release(PrimaryControlState& controls)
{
	controls.pitch.input = 0.0;
	controls.pitch.trim = 0.0;
	controls.roll.input = 0.0;
	controls.roll.trim = 0.0;
	controls.yaw.input = 0.0;
	controls.yaw.trim = 0.0;
}

inline void reset_fbw_throttle_interface(FBWControllerState& fbw)
{
	fbw.throttle_cmd_left = 0.0;
	fbw.throttle_cmd_right = 0.0;
	fbw.throttle_blend = 0.0;
	fbw.throttle_override = false;
}

inline void configure_release(
	StartupSystemState& startup,
	SuspensionSystemState& suspension,
	FBWControllerState& fbw,
	PrimaryControlState& controls,
	WheelState& wheels,
	ThrottleInputState& throttles,
	EngineSystemState& engines,
	double roll,
	double pitch,
	double alpha,
	double g,
	double& elevator_command,
	double& aileron_command,
	double& rudder_command)
{
	reset_suspension_feedback_state(suspension);
	reset_fbw_state(fbw, roll, pitch, alpha, g);
	suspension.on_ground = false;
	startup.simulation_time = 0.0;
	startup.mode = STARTUP_MODE_RELEASED;

	reset_primary_commands_for_release(controls);
	elevator_command = 0.0;
	aileron_command = 0.0;
	rudder_command = 0.0;
	set_nose_turn_enabled(wheels, false);

	reset_throttle_inputs(throttles, 0.0, 0.0);
	reset_fbw_throttle_interface(fbw);
	reset_engine_release_state(engines);
}
}
