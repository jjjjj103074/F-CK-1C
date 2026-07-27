#pragma once

#include "ControlLaws.h"
#include "FlightControlComputerConfig.h"
#include "InputModel.h"
#include "../System.h"
#include "../../Contracts/AircraftData.h"

#include <vector>

namespace Core
{
namespace Systems
{
class FlightControlComputer final : public System
{
public:
	FlightControlComputer(
		const FlightControlComputerConfig& config,
		StartMode start_mode);

	void setup(SystemSetup& setup) override;
	void step(
		const AircraftDataSnapshot& snapshot,
		SystemResult& result) override;

	const FlightControlDemand& step(
		::Systems::FBWControllerInput input,
		const AutopilotCommand& autopilot);
	void handle_command(const Command& command);

	const PilotControlState& pilot_controls() const;
	const FlightControlDemand& demand() const;
	const EngineControlDemand& engine_demand() const;

private:
	void register_commands(SystemSetup& setup);
	void handle_primary_command(const Command& command);
	void handle_yaw_command(const Command& command);
	void handle_fbw_command(const Command& command);
	void handle_throttle_command(const Command& command);
	void apply_autopilot(const AutopilotCommand& autopilot);
	void refresh_pilot_controls();
	void refresh_outputs(const ::Systems::FBWControllerOutput& output);
	double alpha_limit(double mach) const;
	::Systems::FBWControllerInput make_pipeline_input(
		const AircraftDataSnapshot& snapshot) const;

	const FlightControlComputerConfig config_;
	::Systems::PrimaryControlState primary_controls_;
	::Systems::ThrottleInputState throttle_inputs_;
	::Systems::FBWControllerState fbw_;
	PilotControlState pilot_controls_;
	FlightControlDemand demand_;
	EngineControlDemand engine_demand_;
};

SystemEntry make_flight_control_computer_system_entry(
	const FlightControlComputerConfig& config);
}
}
