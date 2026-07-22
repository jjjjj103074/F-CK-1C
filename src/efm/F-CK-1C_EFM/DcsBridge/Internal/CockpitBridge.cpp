#include "CockpitBridge.h"

#include "../../Common/Clamp.h"
#include "../../DcsIds/CockpitParams.g.h"

#include <cmath>
#include <cstring>

namespace
{
constexpr double kEnabledThreshold = 0.5;
constexpr double kMinimumControlCommand = -1.0;
constexpr double kMaximumControlCommand = 1.0;
constexpr double kMinimumThrottleCommand = 0.0;
constexpr double kMaximumThrottleCommand = 1.0;
constexpr double kLegacyCockpitTemperatureOffset = 273.0;
constexpr const char* kMissingHandle = "missing_handle";
constexpr const char* kApiUnavailable = "api_unavailable";
constexpr const char* kUnreadableValue = "unreadable_value";
constexpr const char* kInvalidNumeric = "invalid_numeric";

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
	slots_({ {
		{ DcsIds::CockpitParams::TemperatureC },
		{ DcsIds::CockpitParams::MaxPowerSwitch },
		{ DcsIds::CockpitParams::MaxPowerReady },
		{ DcsIds::CockpitParams::ApMasterEngaged },
		{ DcsIds::CockpitParams::ApPitchCommand },
		{ DcsIds::CockpitParams::ApRollCommand },
		{ DcsIds::CockpitParams::ApThrottleCommand },
		{ DcsIds::CockpitParams::ApBypassActive },
		{ DcsIds::CockpitParams::ApAutoThrottleEngaged }
	} })
{
}

CockpitStepInput CockpitBridge::read_step_input()
{
	std::lock_guard<std::mutex> lock(mutex_);
	CockpitStepInput result;
	result.autopilot = read_autopilot(result.events);
	result.max_power = read_max_power(result.events);
	return result;
}

CockpitParameterEvents CockpitBridge::export_temperature(double dcs_temperature)
{
	std::lock_guard<std::mutex> lock(mutex_);
	CockpitParameterEvents events;
	ParameterSlot& slot = slots_[static_cast<std::size_t>(Parameter::Temperature)];
	if (!ensure_handle(slot))
	{
		record_failure(
			slot,
			{ CockpitParameterEventType::Error, slot.name, kMissingHandle },
			events);
		return events;
	}
	if (api_.pfn_ed_cockpit_update_parameter_with_number == nullptr)
	{
		record_failure(
			slot,
			{ CockpitParameterEventType::Error, slot.name, kApiUnavailable },
			events);
		return events;
	}
	record_available(slot, events);
	api_.pfn_ed_cockpit_update_parameter_with_number(
		slot.handle,
		dcs_temperature + kLegacyCockpitTemperatureOffset);
	return events;
}

bool CockpitBridge::read_parameter(
	Parameter parameter,
	double& value,
	CockpitParameterEvents& events)
{
	ParameterSlot& slot = slots_[static_cast<std::size_t>(parameter)];
	if (!ensure_handle(slot))
	{
		record_failure(
			slot,
			{ CockpitParameterEventType::Error, slot.name, kMissingHandle },
			events);
		return false;
	}
	if (api_.pfn_ed_cockpit_parameter_value_to_number == nullptr)
	{
		record_failure(
			slot,
			{ CockpitParameterEventType::Error, slot.name, kApiUnavailable },
			events);
		return false;
	}
	if (!api_.pfn_ed_cockpit_parameter_value_to_number(slot.handle, value, false))
	{
		record_failure(
			slot,
			{ CockpitParameterEventType::Error, slot.name, kUnreadableValue },
			events);
		return false;
	}
	if (!std::isfinite(value))
	{
		record_failure(
			slot,
			{ CockpitParameterEventType::Error, slot.name, kInvalidNumeric, value, true },
			events);
		return false;
	}
	record_available(slot, events);
	return true;
}

bool CockpitBridge::ensure_handle(ParameterSlot& slot)
{
	if (slot.handle != nullptr)
	{
		return true;
	}
	if (api_.pfn_ed_cockpit_get_parameter_handle == nullptr)
	{
		return false;
	}
	slot.handle = api_.pfn_ed_cockpit_get_parameter_handle(slot.name);
	return slot.handle != nullptr;
}

void CockpitBridge::record_failure(
	ParameterSlot& slot,
	const CockpitParameterEvent& event,
	CockpitParameterEvents& events)
{
	const bool repeated = slot.availability == Availability::Unavailable &&
		slot.failure_reason != nullptr &&
		std::strcmp(slot.failure_reason, event.reason) == 0;
	slot.availability = Availability::Unavailable;
	slot.failure_reason = event.reason;
	if (!repeated && events.count < events.items.size())
	{
		events.items[events.count++] = event;
	}
}

void CockpitBridge::record_available(
	ParameterSlot& slot,
	CockpitParameterEvents& events)
{
	const bool recovered = slot.availability == Availability::Unavailable;
	slot.availability = Availability::Available;
	slot.failure_reason = nullptr;
	if (recovered && events.count < events.items.size())
	{
		events.items[events.count++] = {
			CockpitParameterEventType::Recovery,
			slot.name
		};
	}
}

Core::AutopilotCommand CockpitBridge::read_autopilot(
	CockpitParameterEvents& events)
{
	double master = 0.0;
	double pitch = 0.0;
	double roll = 0.0;
	double throttle = 0.0;
	double bypass = 0.0;
	double auto_throttle = 0.0;
	bool available = read_parameter(Parameter::AutopilotMaster, master, events);
	available = read_parameter(Parameter::AutopilotPitch, pitch, events) && available;
	available = read_parameter(Parameter::AutopilotRoll, roll, events) && available;
	available = read_parameter(Parameter::AutopilotThrottle, throttle, events) && available;
	available = read_parameter(Parameter::AutopilotBypass, bypass, events) && available;
	available = read_parameter(
		Parameter::AutopilotAutoThrottle,
		auto_throttle,
		events) && available;
	if (!available)
	{
		return {};
	}
	Core::AutopilotCommand command;
	command.master = is_enabled(master);
	command.bypass = is_enabled(bypass);
	command.auto_throttle_engaged = is_enabled(auto_throttle);
	if (command.master && !command.bypass)
	{
		command.pitch_command = Common::limit(
			pitch, kMinimumControlCommand, kMaximumControlCommand);
		command.roll_command = Common::limit(
			roll, kMinimumControlCommand, kMaximumControlCommand);
	}
	if (command.auto_throttle_engaged)
	{
		command.throttle_command = Common::limit(
			throttle, kMinimumThrottleCommand, kMaximumThrottleCommand);
	}
	return command;
}

Core::MaxPowerCommand CockpitBridge::read_max_power(
	CockpitParameterEvents& events)
{
	double ready = 0.0;
	double value = 0.0;
	bool available = read_parameter(Parameter::MaxPowerReady, ready, events);
	available = read_parameter(Parameter::MaxPowerSwitch, value, events) && available;
	if (!available)
	{
		return {};
	}
	Core::MaxPowerCommand command;
	command.ready = ready;
	if (command.ready > kEnabledThreshold)
	{
		command.value = is_enabled(value) ? 1.0 : 0.0;
	}
	return command;
}
}
}
