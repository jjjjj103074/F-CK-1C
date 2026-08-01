#pragma once

#include "AutomaticFlightControlTypes.h"
#include "AutopilotModeLogic.h"
#include "AutopilotModeMonitor.h"
#include "ExperimentalAutoThrottleAssist.h"
#include "LateralGuidance.h"
#include "VerticalGuidance.h"
#include "../../../Contracts/CockpitContracts.h"
#include "../../../Contracts/Commands.h"

#include <cstdint>
#include <vector>

namespace Core
{
namespace Systems
{
class SystemSetup;

class AutomaticFlightControl final
{
public:
	AutomaticFlightControl(
		const AutomaticFlightControlConfig& config,
		bool initial_weight_on_wheels);

	void register_commands(SystemSetup& setup);
	void handle_command(const Command& command);
	const AutomaticFlightGuidanceReference& step(
		const AutomaticFlightControlObservation& observation);
	void observe_control_result(
		const AutopilotModeMonitorObservation& observation);
	const AutomaticFlightControlSnapshot& snapshot() const;

private:
	void apply_auto_throttle_commands();
	void synchronize_guidance_lifecycle();
	void update_guidance();
	void refresh_reference();
	void refresh_snapshot();

	const AutomaticFlightControlConfig config_;
	AutopilotModeLogic mode_logic_;
	VerticalGuidance vertical_guidance_;
	LateralGuidance lateral_guidance_;
	ExperimentalAutoThrottleAssist auto_throttle_;
	AutopilotModeMonitor mode_monitor_;
	AutomaticFlightControlObservation observation_;
	AutomaticFlightGuidanceReference reference_;
	AutomaticFlightControlSnapshot snapshot_;
	std::vector<Command> pending_auto_throttle_commands_;
	AutopilotModeMonitorResult pending_monitor_result_;
	VerticalGuidanceReference vertical_reference_;
	LateralGuidanceReference lateral_reference_;
	std::uint64_t applied_vertical_revision_ = 0;
	std::uint64_t applied_lateral_revision_ = 0;
	std::uint64_t revision_ = 0;
	ConstraintReason constraint_reason_ = ConstraintReason::None;
};

AutomaticFlightControlConfig fck1c_automatic_flight_control_config();
void validate_automatic_flight_control_config(
	const AutomaticFlightControlConfig& config);
}
}
