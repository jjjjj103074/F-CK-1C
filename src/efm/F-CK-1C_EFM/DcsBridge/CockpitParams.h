#pragma once

#include "../DcsIds/CockpitParams.h"
#include "../include/Cockpit/CockpitAPI_Declare.h"

namespace DcsBridge
{
struct CockpitParamHandles
{
	void* temperature;
	void* maxpower_switch;
	void* maxpower_ready;
};

struct MaxPowerSwitchState
{
	double ready;
	double value;
};

inline CockpitParamHandles make_cockpit_param_handles(EDPARAM& interface)
{
	CockpitParamHandles handles = {
		interface.getParamHandle(DcsIds::CockpitParams::TemperatureC),
		interface.getParamHandle(DcsIds::CockpitParams::MaxPowerSwitch),
		interface.getParamHandle(DcsIds::CockpitParams::MaxPowerReady)
	};

	return handles;
}

inline void export_temperature_param(EDPARAM& interface, const CockpitParamHandles& handles, double value)
{
	interface.setParamNumber(handles.temperature, value);
}

inline MaxPowerSwitchState read_max_power_switch(EDPARAM& interface, const CockpitParamHandles& handles)
{
	MaxPowerSwitchState state = { 0.0, 1.0 };

	if (handles.maxpower_ready != nullptr)
	{
		state.ready = interface.getParamNumber(handles.maxpower_ready);
	}

	if (state.ready > 0.5 && handles.maxpower_switch != nullptr)
	{
		state.value = interface.getParamNumber(handles.maxpower_switch) > 0.5 ? 1.0 : 0.0;
	}

	return state;
}
}
