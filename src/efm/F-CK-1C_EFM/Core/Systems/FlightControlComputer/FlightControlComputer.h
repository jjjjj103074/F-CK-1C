#pragma once

#include "ControlLaws.h"
#include "InputModel.h"
#include "../System.h"
#include "../../Contracts/AircraftData.h"

#include <vector>

namespace Core
{
namespace Systems
{
struct FlightEnvelopeDefinition
{
	const std::vector<double>& mach;
	const std::vector<double>& alpha_limit_deg;
};

class FlightControlComputer final : public System
{
public:
	FlightControlComputer(
		const ::Systems::FBWControllerConfig& config,
		const FlightEnvelopeDefinition& envelope,
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

	const ::Systems::FBWControllerConfig& config_;
	const std::vector<double>& mach_table_;
	const std::vector<double>& alpha_limit_table_;
	::Systems::PrimaryControlState primary_controls_;
	::Systems::ThrottleInputState throttle_inputs_;
	::Systems::FBWControllerState fbw_;
	PilotControlState pilot_controls_;
	FlightControlDemand demand_;
	EngineControlDemand engine_demand_;
};
}
}
