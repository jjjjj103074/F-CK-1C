#pragma once

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

inline void begin_startup(StartupSystemState& state, StartupMode mode)
{
	state.mode = mode;
	state.simulation_time = 0.0;
	state.first_frame_completed = false;
}

inline void configure_release(StartupSystemState& state)
{
	state.simulation_time = 0.0;
	state.mode = STARTUP_MODE_RELEASED;
	state.first_frame_completed = false;
}
}
