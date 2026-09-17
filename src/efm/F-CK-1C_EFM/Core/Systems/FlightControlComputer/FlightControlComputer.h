#pragma once

#include "Configuration/FlightControlComputerConfig.h"
#include "Diagnostics/FlightControlComputerDebugTelemetry.h"
#include "FlightControlExecutive.h"
#include "../System.h"

namespace Core
{
namespace Systems
{
// FLCC 對外部 SystemPipeline 的唯一 System 介面。
// 負責資料鍵、更新率與診斷輸出的銜接；內部運算順序交由 Executive 管理。
class FlightControlComputer final : public System
{
public:
	FlightControlComputer(
		const FlightControlComputerConfig& config,
		StartMode start_mode,
		const ThrottleLeverSignal& initial_throttle_levers);
	void setup(SystemSetup& setup) override;
	void step(
		const SystemStepContext& context,
		const AircraftDataView& aircraft,
		SystemResult& result) override;
	const FlightControlActuatorCommand& step(
		const FlightControlComputerStepInput& input);
	void handle_command(const Command& command);
	const FlightControlActuatorCommand& actuator_command() const;
	const EngineThrottleCommand& engine_throttle_command() const;
	const FlightControlComputerSnapshot& diagnostics() const;
	const AutomaticFlightControlSnapshot& automatic_flight_control_snapshot()
		const;

private:
	RawFlightControlInput make_pipeline_input(
		const SystemStepContext& context,
		const AircraftDataView& aircraft) const;

	FlightControlExecutive executive_;
	FlightControlComputerDebugTelemetry debug_telemetry_;
};

SystemEntry make_flight_control_computer_system_entry(
	const FlightControlComputerConfig& config);
}
}
