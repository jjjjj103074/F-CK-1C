#pragma once

#include "CommandSystem/FlightControlCommandSystem.h"
#include "Configuration/FlightControlComputerConfig.h"
#include "Diagnostics/FlightControlDiagnostics.h"
#include "Output/FlightControlOutputSystem.h"
#include "Util/FlightStateComputation.h"
#include "Input/InputSignalManagement.h"
#include "Util/ThrottleCommandComposition.h"
#include "ControlLaws/ControlLaws.h"

namespace Core
{
namespace Systems
{
// Executive 的單次輸入與結果；外層 System 負責讀寫 AircraftData。
struct FlightControlComputerStepInput
{
	RawFlightControlInput flight_control;
	ThrottleLeverSignal throttle_levers;
};

struct FlightControlComputerResult
{
	FlightControlActuatorCommand actuator_command;
	EngineThrottleCommand engine_throttle_command;
	FlightControlComputerSnapshot diagnostics;
	AutomaticFlightControlSnapshot automatic_flight_control;
};

struct FlightControlExecutiveDiagnosticsInput
{
	const FlightControlCommandSystemResult& command;
	const ::Systems::FlightControlLawsResult& laws;
	const ::Systems::ActiveFlightControlConfiguration& configuration;
	const ::Systems::ManagedFlightControlSignals& signals;
	const FlightControlOutputStatus& output_status;
};

struct AutopilotMonitorInput
{
	const ::Systems::ComputedFlightState& flight;
	const FlightControlCommandSystemResult& command;
	const ::Systems::FlightControlLawsResult& laws;
	bool control_path_saturated = false;
};

// FLCC 內部的執行鏈：擁有計算模組，決定呼叫順序與子頻率。
// 它不是另一個飛機 System，也不直接發布資料到外部 SystemPipeline。
class FlightControlExecutive
{
public:
	FlightControlExecutive(
		const FlightControlComputerConfig& config,
		StartMode start_mode,
		const ThrottleLeverSignal& initial_throttle_levers);
	// 唯一需要 SystemSetup 的內部介面：將指令處理器接到外部排程器。
	void register_commands(SystemSetup& setup);
	void handle_command(const Command& command);
	const FlightControlComputerResult& update(
		const FlightControlComputerStepInput& input);
	const FlightControlComputerResult& result() const;

private:
	void update_subrate_counters(bool shaping_tick, bool gain_tick);
	void update_engine_throttle(
		const ThrottleLeverSignal& levers,
		const AutomaticFlightGuidanceReference& automatic);
	void update_diagnostics(
		const FlightControlExecutiveDiagnosticsInput& input);
	FlightControlCommandMonitorInput make_monitor_observation(
		const AutopilotMonitorInput& input) const;
	ConstraintReason hard_protection_reason(
		const ::Systems::FlightControlLawsStatus& status) const;
	const FlightControlComputerConfig config_;
	InputSignalManagement input_signals_;
	::Systems::ModeAndGainScheduling mode_and_gain_;
	FlightControlCommandSystem command_system_;
	::Systems::FlightControlLaws flight_control_laws_;
	FlightControlOutputSystem output_system_;
	FlightControlDiagnostics diagnostics_;
	// 慢頻增益排程的最近一次結果，供下一次輸入塑形更新使用。
	::Systems::PilotInputShapingConfig held_pilot_shaping_;
	FlightControlComputerResult result_;
	bool developer_g_limiter_override_active_ = false;
	std::uint64_t tick_ = 0;
	std::uint64_t pilot_shaping_update_count_ = 0;
	std::uint64_t pilot_shaping_last_update_tick_ = 0;
	std::uint64_t gain_schedule_update_count_ = 0;
	std::uint64_t gain_schedule_last_update_tick_ = 0;
};
}
}
