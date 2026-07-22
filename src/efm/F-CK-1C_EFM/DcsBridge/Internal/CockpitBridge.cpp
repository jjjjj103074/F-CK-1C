#include "CockpitBridge.h"

#include "../../Common/Clamp.h"
#include "../../DcsIds/CockpitParams.g.h"

namespace
{
constexpr double kEnabledThreshold = 0.5;
constexpr double kMinimumControlCommand = -1.0;
constexpr double kMaximumControlCommand = 1.0;
constexpr double kMinimumThrottleCommand = 0.0;
constexpr double kMaximumThrottleCommand = 1.0;
constexpr double kLegacyCockpitTemperatureOffset = 273.0;

bool is_enabled(double value)
{
	return value > kEnabledThreshold;
}
}

namespace DcsBridge
{
namespace Internal
{
CockpitBridge::CockpitBridge(cockpit_param_api api)
	: api_(api),
	handles_({
		api_.pfn_ed_cockpit_get_parameter_handle(DcsIds::CockpitParams::TemperatureC),
		api_.pfn_ed_cockpit_get_parameter_handle(DcsIds::CockpitParams::MaxPowerSwitch),
		api_.pfn_ed_cockpit_get_parameter_handle(DcsIds::CockpitParams::MaxPowerReady),
		api_.pfn_ed_cockpit_get_parameter_handle(DcsIds::CockpitParams::ApMasterEngaged),
		api_.pfn_ed_cockpit_get_parameter_handle(DcsIds::CockpitParams::ApPitchCommand),
		api_.pfn_ed_cockpit_get_parameter_handle(DcsIds::CockpitParams::ApRollCommand),
		api_.pfn_ed_cockpit_get_parameter_handle(DcsIds::CockpitParams::ApThrottleCommand),
		api_.pfn_ed_cockpit_get_parameter_handle(DcsIds::CockpitParams::ApBypassActive),
		api_.pfn_ed_cockpit_get_parameter_handle(DcsIds::CockpitParams::ApAutoThrottleEngaged)
	})
{
}

Core::AutopilotCommand CockpitBridge::read_autopilot() const
{
	if (!autopilot_parameters_available())
	{
		return {};
	}
	Core::AutopilotCommand command;
	command.master = is_enabled(read_number(handles_.autopilot_master));
	command.bypass = is_enabled(read_number(handles_.autopilot_bypass));
	command.auto_throttle_engaged =
		is_enabled(read_number(handles_.autopilot_auto_throttle));
	if (command.master && !command.bypass)
	{
		command.pitch_command = Common::limit(
			read_number(handles_.autopilot_pitch),
			kMinimumControlCommand,
			kMaximumControlCommand);
		command.roll_command = Common::limit(
			read_number(handles_.autopilot_roll),
			kMinimumControlCommand,
			kMaximumControlCommand);
	}
	if (command.auto_throttle_engaged)
	{
		command.throttle_command = Common::limit(
			read_number(handles_.autopilot_throttle),
			kMinimumThrottleCommand,
			kMaximumThrottleCommand);
	}
	return command;
}

Core::MaxPowerCommand CockpitBridge::read_max_power() const
{
	Core::MaxPowerCommand command;
	if (handles_.max_power_ready == nullptr)
	{
		return command;
	}
	command.ready = read_number(handles_.max_power_ready);
	if (command.ready > kEnabledThreshold && handles_.max_power_switch != nullptr)
	{
		command.value = is_enabled(read_number(handles_.max_power_switch)) ? 1.0 : 0.0;
	}
	return command;
}

void CockpitBridge::export_temperature(double dcs_temperature) const
{
	api_.pfn_ed_cockpit_update_parameter_with_number(
		handles_.temperature,
		dcs_temperature + kLegacyCockpitTemperatureOffset);
}

bool CockpitBridge::autopilot_parameters_available() const
{
	return handles_.autopilot_master != nullptr &&
		handles_.autopilot_pitch != nullptr &&
		handles_.autopilot_roll != nullptr &&
		handles_.autopilot_throttle != nullptr &&
		handles_.autopilot_bypass != nullptr &&
		handles_.autopilot_auto_throttle != nullptr;
}

double CockpitBridge::read_number(void* handle) const
{
	double value = 0.0;
	(void)api_.pfn_ed_cockpit_parameter_value_to_number(handle, value, false);
	return value;
}
}
}
