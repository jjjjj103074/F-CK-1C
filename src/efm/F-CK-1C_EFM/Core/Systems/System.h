#pragma once

#include "../Contracts/Commands.h"
#include "../Contracts/AircraftData.h"
#include "../Contracts/Diagnostics/DebugTelemetry.h"
#include "../Contracts/Events.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Core
{
namespace Systems
{
class AircraftDataView;
class SystemResult;
class SystemSetup;

using SystemScheduledTime = DebugSimulationTime;

struct SystemStepContext
{
	SystemScheduledTime scheduled_time = {};
	double dt_s = 0.0;
};

struct SystemActionContext
{
	SystemScheduledTime simulation_time = {};
};

struct FlightFuelState
{
	double internal_fuel_kg = 0.0;
	std::vector<ExternalFuelInput> external_fuel;
};

struct FlightSetupContext
{
	const StartMode start_mode;
	const FlightFuelState fuel;
	const ThrottleLeverSignal initial_throttle_levers;
	DebugTelemetrySink& debug_telemetry;
};

class System
{
public:
	virtual ~System() = default;

	virtual void setup(SystemSetup& setup) = 0;
	virtual void step(
		const SystemStepContext& context,
		const AircraftDataView& aircraft,
		SystemResult& result) = 0;
};

using SystemFactory =
	std::function<std::unique_ptr<System>(const FlightSetupContext&)>;

struct SystemEntry
{
	std::string id;  // Pipeline 識別系統、檢查重複及標示錯誤時使用。
	SystemFactory factory;  // 建立每架飛機實例時才呼叫的工廠。
};
}
}
