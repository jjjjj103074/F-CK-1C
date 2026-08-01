#pragma once

#include "AutomaticFlightControlTypes.h"
#include "../../../Contracts/CockpitContracts.h"
#include "../../../Contracts/Commands.h"

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
		const AutomaticFlightControlConfig& config);
	bool handle_command(
		const Command& command,
		const AutomaticFlightControlObservation& observation);
	const ExperimentalAutoThrottleResult& update(
		const AutomaticFlightControlObservation& observation);
	const ExperimentalAutoThrottleResult& result() const;
	void disengage(AutomaticFlightControlReason reason);

private:
	bool can_engage(const AutomaticFlightControlObservation& observation);
	void engage(const AutomaticFlightControlObservation& observation);
	void adjust_speed_reference(double direction);
	void reset_controller();

	const AutomaticFlightControlConfig config_;
	ExperimentalAutoThrottleResult result_;
	double speed_integral_ = 0.0;
};
}
}
