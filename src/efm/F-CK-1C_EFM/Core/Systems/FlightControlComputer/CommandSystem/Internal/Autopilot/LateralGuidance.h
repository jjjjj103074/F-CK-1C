#pragma once

#include "../../AutomaticFlightControlTypes.h"
#include "AutomaticFlightControlObservation.h"

namespace Core
{
namespace Systems
{
struct LateralGuidanceReference
{
	double bank_angle_rad = 0.0;
};

struct LateralGuidanceStepInput
{
	AutomaticFlightControlObservation observation;
	AutomaticFlightControlLateralMode mode =
		AutomaticFlightControlLateralMode::Off;
	double target_heading_deg = 0.0;
	double target_roll_rad = 0.0;
	bool active = false;
	bool constrained = false;
};

class LateralGuidance
{
public:
	LateralGuidance(
		const AutomaticFlightControlConfig& config,
		const AutomaticFlightGuidanceLimits& limits);
	LateralGuidanceReference update(const LateralGuidanceStepInput& input);
	void track_observation(const AutomaticFlightControlObservation& observation);
	void reset();

private:
	double heading_select_bank(const LateralGuidanceStepInput& input);
	double rate_limit_bank(
		double desired_bank_rad,
		const AutomaticFlightControlObservation& observation);
	const AutomaticFlightControlConfig config_;
	const AutomaticFlightGuidanceLimits limits_;
	double heading_error_integral_deg_s_ = 0.0;
	double bank_reference_rad_ = 0.0;
	bool active_last_tick_ = false;
	AutomaticFlightControlLateralMode previous_mode_ =
		AutomaticFlightControlLateralMode::Off;
};
}
}
