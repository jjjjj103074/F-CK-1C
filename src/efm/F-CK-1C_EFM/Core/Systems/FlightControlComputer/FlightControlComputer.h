#pragma once

#include "Autopilot/AutomaticFlightControl.h"
#include "ControlLaws/ControlReferenceSelection.h"
#include "ControlLaws/GuidanceCoordination.h"
#include "ControlLaws/ControlLaws.h"
#include "FlightControlComputerConfig.h"
#include "InputSignalManagement.h"
#include "ThrottleCommandComposition.h"
#include "../System.h"
#include "../../Contracts/AircraftData.h"

namespace Core
{
namespace Systems
{
struct FlightControlComputerStepInput
{
	RawFlightControlInput flight_control;
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
	::Systems::FlightControlLawStepInput make_control_law_input(
		const ::Systems::ConditionedFlightControlInput& flight,
		const AutomaticFlightGuidanceReference& automatic);
	void apply_experimental_auto_throttle(
		const AutomaticFlightGuidanceReference& automatic);
	void refresh_outputs(
		const ::Systems::FlightControlLawResult& output,
		const ThrottleLeverSignal& throttle_levers);
	void refresh_diagnostics();
	AutomaticFlightControlObservation make_automatic_observation(
		const RawFlightControlInput& raw,
		const ::Systems::ConditionedFlightControlInput& conditioned) const;
	double alpha_limit(double mach) const;
	RawFlightControlInput make_pipeline_input(
		const SystemStepContext& context,
		const AircraftDataView& aircraft) const;

	const FlightControlComputerConfig config_;
	InputSignalManagement input_signals_;
	::Systems::FBWControllerState fbw_;
	AutomaticFlightControl automatic_flight_control_;
	SelectedFlightReference selected_reference_;
	GuidanceCoordinationResult coordinated_reference_;
	FlightControlActuatorCommand actuator_command_;
	EngineThrottleCommand engine_throttle_command_;
	FlightControlComputerSnapshot diagnostics_;
};

SystemEntry make_flight_control_computer_system_entry(
	const FlightControlComputerConfig& config);
}
}
