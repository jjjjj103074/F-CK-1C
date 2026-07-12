#pragma once

#include "../Common/Clamp.h"
#include "../DcsIds/CockpitParams.h"
#include "../include/Cockpit/CockpitAPI_Declare.h"

namespace DcsBridge
{
struct AutopilotParamHandles
{
	void* master;
	void* pitch_cmd;
	void* roll_cmd;
	void* throttle_cmd;
	void* bypass;
	void* at_engaged;
};

struct AutopilotState
{
	double pitch_cmd;
	double roll_cmd;
	double throttle_cmd;
	bool master;
	bool bypass;
	bool at_engaged;
};

inline AutopilotParamHandles make_autopilot_param_handles(EDPARAM& interface)
{
	AutopilotParamHandles handles = {
		interface.getParamHandle(DcsIds::CockpitParams::ApMasterEngaged),
		interface.getParamHandle(DcsIds::CockpitParams::ApPitchCommand),
		interface.getParamHandle(DcsIds::CockpitParams::ApRollCommand),
		interface.getParamHandle(DcsIds::CockpitParams::ApThrottleCommand),
		interface.getParamHandle(DcsIds::CockpitParams::ApBypassActive),
		interface.getParamHandle(DcsIds::CockpitParams::ApAutoThrottleEngaged)
	};

	return handles;
}

inline void reset_autopilot_state(AutopilotState& state)
{
	state.pitch_cmd = 0.0;
	state.roll_cmd = 0.0;
	state.throttle_cmd = 0.0;
	state.master = false;
	state.bypass = false;
	state.at_engaged = false;
}

inline void update_autopilot_from_lua(EDPARAM& interface, const AutopilotParamHandles& handles, AutopilotState& state)
{
	state.master = interface.getParamNumber(handles.master) > 0.5;
	state.bypass = interface.getParamNumber(handles.bypass) > 0.5;
	state.at_engaged = interface.getParamNumber(handles.at_engaged) > 0.5;

	if (state.master && !state.bypass)
	{
		state.pitch_cmd = Common::limit(interface.getParamNumber(handles.pitch_cmd), -1.0, 1.0);
		state.roll_cmd = Common::limit(interface.getParamNumber(handles.roll_cmd), -1.0, 1.0);
	}
	else
	{
		state.pitch_cmd = 0.0;
		state.roll_cmd = 0.0;
	}

	if (state.at_engaged)
	{
		state.throttle_cmd = Common::limit(interface.getParamNumber(handles.throttle_cmd), 0.0, 1.0);
	}
	else
	{
		state.throttle_cmd = 0.0;
	}
}
}
