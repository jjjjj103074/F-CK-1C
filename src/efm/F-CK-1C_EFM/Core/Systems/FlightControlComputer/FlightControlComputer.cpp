#include "FlightControlComputer.h"

#include "../SystemPipeline.h"
#include "../SystemUpdateRates.h"

namespace Core
{
namespace Systems
{
FlightControlComputer::FlightControlComputer(
	const FlightControlComputerConfig& config,
	StartMode start_mode,
	const ThrottleLeverSignal& initial_throttle_levers)
	: executive_(config, start_mode, initial_throttle_levers)
{
}

void FlightControlComputer::setup(SystemSetup& setup)
{
	// 向外部排程器宣告 FLCC 的週期、輸入、輸出與可處理的指令。
	debug_telemetry_.declare_channels(setup);
	setup.update_rate_hz(kF16XlDflcsReferenceUpdateRateHz);
	setup.read(AircraftDataKeys::kFlightControlObservation);
	setup.read(AircraftDataKeys::kPilotControlSignal);
	setup.read(AircraftDataKeys::kThrottleLeverSignal);
	setup.read(AircraftDataKeys::kLandingGearData);
	setup.read(AircraftDataKeys::kFlightControlActuatorState);
	const FlightControlComputerResult& initial = executive_.result();
	setup.publish(
		AircraftDataKeys::kFlightControlActuatorCommand,
		initial.actuator_command);
	setup.publish(
		AircraftDataKeys::kEngineThrottleCommand,
		initial.engine_throttle_command);
	setup.publish(
		AircraftDataKeys::kAutomaticFlightControlSnapshot,
		initial.automatic_flight_control);
	setup.publish(
		AircraftDataKeys::kFlightControlComputerSnapshot,
		initial.diagnostics);
	executive_.register_commands(setup);
	debug_telemetry_.publish_initial(
		initial.diagnostics, initial.automatic_flight_control);
}

void FlightControlComputer::step(
	const SystemStepContext& context,
	const AircraftDataView& aircraft,
	SystemResult& result)
{
	// 先把 SystemPipeline 的資料整理成單次運算輸入；此層不計算控制律。
	const RawFlightControlInput flight = make_pipeline_input(context, aircraft);
	const FlightControlComputerResult& output = executive_.update({
		flight,
		aircraft.read(AircraftDataKeys::kThrottleLeverSignal)
	});
	debug_telemetry_.publish_step({ context.scheduled_time,
		flight.observation, output.diagnostics,
		output.automatic_flight_control });
	// Executive 完成本次運算後，才將結果發布給其他飛機系統。
	result.publish(AircraftDataKeys::kFlightControlActuatorCommand,
		output.actuator_command);
	result.publish(AircraftDataKeys::kEngineThrottleCommand,
		output.engine_throttle_command);
	result.publish(AircraftDataKeys::kAutomaticFlightControlSnapshot,
		output.automatic_flight_control);
	result.publish(AircraftDataKeys::kFlightControlComputerSnapshot,
		output.diagnostics);
}

const FlightControlActuatorCommand& FlightControlComputer::step(
	const FlightControlComputerStepInput& input)
{
	return executive_.update(input).actuator_command;
}

void FlightControlComputer::handle_command(const Command& command)
{
	executive_.handle_command(command);
}

RawFlightControlInput FlightControlComputer::make_pipeline_input(
	const SystemStepContext& context,
	const AircraftDataView& aircraft) const
{
	return {
		context.dt_s,
		aircraft.read(AircraftDataKeys::kFlightControlObservation),
		aircraft.read(AircraftDataKeys::kPilotControlSignal),
		aircraft.read(AircraftDataKeys::kLandingGearData),
		aircraft.read(AircraftDataKeys::kFlightControlActuatorState)
	};
}

const FlightControlActuatorCommand&
FlightControlComputer::actuator_command() const
{
	return executive_.result().actuator_command;
}

const EngineThrottleCommand& FlightControlComputer::engine_throttle_command()
	const
{
	return executive_.result().engine_throttle_command;
}

const FlightControlComputerSnapshot& FlightControlComputer::diagnostics() const
{
	return executive_.result().diagnostics;
}

const AutomaticFlightControlSnapshot&
FlightControlComputer::automatic_flight_control_snapshot() const
{
	return executive_.result().automatic_flight_control;
}
}
}
