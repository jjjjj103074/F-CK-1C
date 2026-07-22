#pragma once

#include "../../Core/FrameContracts.h"
#include "../../include/Cockpit/ccParametersAPI.h"

#include <array>
#include <cstddef>
#include <mutex>

namespace DcsBridge
{
namespace Internal
{
enum class CockpitParameterEventType
{
	Error,
	Recovery
};

struct CockpitParameterEvent
{
	CockpitParameterEventType type = CockpitParameterEventType::Error;
	const char* parameter_name = nullptr;
	const char* reason = nullptr;
	double value = 0.0;
	bool has_value = false;
};

inline constexpr std::size_t kCockpitParameterEventCapacity = 9;

struct CockpitParameterEvents
{
	std::array<CockpitParameterEvent, kCockpitParameterEventCapacity> items = {};
	std::size_t count = 0;
};

struct CockpitStepInput
{
	Core::AutopilotCommand autopilot;
	Core::MaxPowerCommand max_power;
	CockpitParameterEvents events;
};

class CockpitBridge final
{
public:
	explicit CockpitBridge(cockpit_param_api api);

	CockpitBridge(const CockpitBridge&) = delete;
	CockpitBridge& operator=(const CockpitBridge&) = delete;

	CockpitStepInput read_step_input();
	CockpitParameterEvents export_temperature(double dcs_temperature);

private:
	enum class Parameter : std::size_t
	{
		Temperature,
		MaxPowerSwitch,
		MaxPowerReady,
		AutopilotMaster,
		AutopilotPitch,
		AutopilotRoll,
		AutopilotThrottle,
		AutopilotBypass,
		AutopilotAutoThrottle,
		Count
	};

	enum class Availability
	{
		Unknown,
		Available,
		Unavailable
	};

	struct ParameterSlot
	{
		const char* name = nullptr;
		void* handle = nullptr;
		Availability availability = Availability::Unknown;
		const char* failure_reason = nullptr;
	};

	bool read_parameter(
		Parameter parameter,
		double& value,
		CockpitParameterEvents& events);
	bool ensure_handle(ParameterSlot& slot);
	void record_failure(
		ParameterSlot& slot,
		const CockpitParameterEvent& event,
		CockpitParameterEvents& events);
	void record_available(
		ParameterSlot& slot,
		CockpitParameterEvents& events);
	Core::AutopilotCommand read_autopilot(CockpitParameterEvents& events);
	Core::MaxPowerCommand read_max_power(CockpitParameterEvents& events);

	const cockpit_param_api api_;
	std::array<ParameterSlot, static_cast<std::size_t>(Parameter::Count)> slots_;
	std::mutex mutex_;
};
}
}
