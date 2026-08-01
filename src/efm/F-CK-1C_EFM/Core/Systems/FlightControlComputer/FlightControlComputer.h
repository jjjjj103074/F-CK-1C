#pragma once

#include "Autopilot/AutomaticFlightControl.h"
#include "ControlLaws/ControlLaws.h"
#include "FlightControlComputerConfig.h"
#include "ThrottleCommandComposition.h"
#include "../System.h"
#include "../../Contracts/AircraftData.h"

namespace Core
{
namespace Systems
{
struct FlightControlComputerStepInput
{
	::Systems::FBWControllerInput flight_control;
	AutomaticFlightControlDemand automatic;
	PilotControlSignal pilot;
	ThrottleLeverSignal throttle_levers;
};

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

private:
	void register_commands(SystemSetup& setup);
	::Systems::FBWControllerInput apply_pilot_signal(
		const ::Systems::FBWControllerInput& input,
		const PilotControlSignal& pilot) const;
	void apply_automatic_flight_control(
		::Systems::FBWControllerInput& input,
		const AutomaticFlightControlDemand& automatic);
	void refresh_outputs(
		const ::Systems::FBWControllerOutput& output,
		const ThrottleLeverSignal& throttle_levers);
	void refresh_diagnostics();
	AutomaticFlightControlObservation make_automatic_observation(
		const SystemStepContext& context,
		const AircraftDataView& aircraft) const;
	double alpha_limit(double mach) const;
	::Systems::FBWControllerInput make_pipeline_input(
		const SystemStepContext& context,
		const AircraftDataView& aircraft) const;

	const FlightControlComputerConfig config_;
	::Systems::FBWControllerState fbw_;
	AutomaticFlightControl automatic_flight_control_;
	FlightControlActuatorCommand actuator_command_;
	EngineThrottleCommand engine_throttle_command_;
	FlightControlComputerSnapshot diagnostics_;
};

SystemEntry make_flight_control_computer_system_entry(
	const FlightControlComputerConfig& config);
}
}
