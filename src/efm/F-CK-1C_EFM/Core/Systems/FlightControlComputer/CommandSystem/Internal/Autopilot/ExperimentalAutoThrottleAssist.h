#pragma once

#include "../../AutomaticFlightControlTypes.h"
#include "AutomaticFlightControlObservation.h"
#include "../../../../../Contracts/CockpitContracts.h"
#include "../../../../../Contracts/Commands.h"

namespace Core
{
namespace Systems
{
struct ExperimentalAutoThrottleResult
{
	bool engaged = false;
	double throttle_normalized = 0.0;
	double target_speed_mps = 0.0;
	AutomaticFlightControlReason engage_rejection_reason =
		AutomaticFlightControlReason::None;
	AutomaticFlightControlReason disengage_reason =
		AutomaticFlightControlReason::None;
};

class ExperimentalAutoThrottleAssist
{
public:
	explicit ExperimentalAutoThrottleAssist(
		const ExperimentalAutoThrottleAssistConfig& config);
	bool handle_command(
		const Command& command,
		const AutomaticFlightControlObservation& observation);
	const ExperimentalAutoThrottleResult& update(
		const AutomaticFlightControlObservation& observation);
	const ExperimentalAutoThrottleResult& result() const;
	void disengage(AutomaticFlightControlReason reason);

private:
	void toggle(const AutomaticFlightControlObservation& observation);
	void engage_if_needed(
		const AutomaticFlightControlObservation& observation);
	void adjust_speed_if_engaged(double direction);
	bool can_engage(const AutomaticFlightControlObservation& observation);
	void engage(const AutomaticFlightControlObservation& observation);
	void adjust_speed_reference(double direction);
	void reset_controller();

	const ExperimentalAutoThrottleAssistConfig config_;
	ExperimentalAutoThrottleResult result_;
	double speed_integral_m_ = 0.0;
};
}
}
