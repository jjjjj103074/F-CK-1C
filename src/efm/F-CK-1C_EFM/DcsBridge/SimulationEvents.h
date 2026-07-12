#pragma once

#include "../include/FM/wHumanCustomPhysicsAPI.h"

namespace DcsBridge
{
struct CarrierLaunchState
{
	int phase;
};

inline void reset_carrier_launch_state(CarrierLaunchState& state)
{
	state.phase = 0;
}

inline bool pop_carrier_launch_event(
	CarrierLaunchState& state,
	ed_fm_simulation_event& out,
	double left_throttle_output,
	double launch_engine_thrust)
{
	// Catapult launch sequence.
	if (state.phase == 1)
	{
		if (left_throttle_output > 0.99) // Automatic launch at full throttle
		{
			out.event_type = ED_FM_EVENT_CARRIER_CATAPULT;
			out.event_params[0] = 1;
			out.event_params[1] = 2.0; // Start delay (s)
			out.event_params[2] = 80.0; // Added velocity after takeoff (m/s)
			out.event_params[3] = launch_engine_thrust; // Engine thrust during takeoff (N)? Doesn't seem to work.
			state.phase = 2;
			return true;
		}
	}

	return false;
}

inline bool push_carrier_launch_event(CarrierLaunchState& state, const ed_fm_simulation_event& in)
{
	if (in.event_type == ED_FM_EVENT_CARRIER_CATAPULT)
	{
		if (in.event_params[0] == 1)
		{
			state.phase = 1;
		}
		else if (in.event_params[0] == 2) // start launch
		{
			state.phase = 3;
		}
		else if (in.event_params[0] == 3) // launch finished
		{
			state.phase = 0;
		}
	}

	return false;
}
}
