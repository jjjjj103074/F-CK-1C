#pragma once

#include "../include/FM/wHumanCustomPhysicsAPI.h"

namespace DcsBridge
{
enum class CarrierLaunchPhase
{
	Idle = 0,
	Armed = 1,
	Issued = 2,
	Started = 3
};

struct CarrierLaunchState
{
	CarrierLaunchPhase phase = CarrierLaunchPhase::Idle;
};

struct CarrierLaunchRequest
{
	double left_throttle_output = 0.0;
	double launch_engine_thrust = 0.0;
};

constexpr double kCarrierLaunchThrottleThreshold = 0.99;
constexpr double kCarrierLaunchStartDelay = 2.0;
constexpr double kCarrierLaunchAddedVelocity = 80.0;

inline void reset_carrier_launch_state(CarrierLaunchState& state)
{
	state.phase = CarrierLaunchPhase::Idle;
}

inline bool pop_carrier_launch_event(
	CarrierLaunchState& state,
	ed_fm_simulation_event& out,
	const CarrierLaunchRequest& request)
{
	// Catapult launch sequence.
	if (state.phase != CarrierLaunchPhase::Armed ||
		request.left_throttle_output <= kCarrierLaunchThrottleThreshold)
	{
		return false;
	}
	out.event_type = ED_FM_EVENT_CARRIER_CATAPULT;
	out.event_params[0] = 1;
	out.event_params[1] = kCarrierLaunchStartDelay;
	out.event_params[2] = kCarrierLaunchAddedVelocity;
	out.event_params[3] = request.launch_engine_thrust;
	state.phase = CarrierLaunchPhase::Issued;
	return true;
}

inline bool push_carrier_launch_event(CarrierLaunchState& state, const ed_fm_simulation_event& in)
{
	if (in.event_type != ED_FM_EVENT_CARRIER_CATAPULT)
	{
		return false;
	}
	const double event_phase = in.event_params[0];
	if (event_phase == 1.0)
	{
		state.phase = CarrierLaunchPhase::Armed;
	}
	else if (event_phase == 2.0)
	{
		state.phase = CarrierLaunchPhase::Started;
	}
	else if (event_phase == 3.0)
	{
		state.phase = CarrierLaunchPhase::Idle;
	}
	return false;
}
}
