#pragma once

#include "../../Core/FrameContracts.h"
#include "../../include/Cockpit/ccParametersAPI.h"

namespace DcsBridge
{
namespace Internal
{
class CockpitBridge final
{
public:
	explicit CockpitBridge(cockpit_param_api api);

	CockpitBridge(const CockpitBridge&) = delete;
	CockpitBridge& operator=(const CockpitBridge&) = delete;

	Core::AutopilotCommand read_autopilot() const;
	Core::MaxPowerCommand read_max_power() const;
	void export_temperature(double dcs_temperature) const;

private:
	struct ParameterHandles
	{
		void* temperature;
		void* max_power_switch;
		void* max_power_ready;
		void* autopilot_master;
		void* autopilot_pitch;
		void* autopilot_roll;
		void* autopilot_throttle;
		void* autopilot_bypass;
		void* autopilot_auto_throttle;
	};

	bool autopilot_parameters_available() const;
	double read_number(void* handle) const;

	const cockpit_param_api api_;
	const ParameterHandles handles_;
};
}
}
