#pragma once

#include "ControlLawSignals.h"

namespace Systems
{
struct BodyRateReference
{
	double roll_rate_rad_s = 0.0;
	double pitch_rate_rad_s = 0.0;
	double yaw_rate_rad_s = 0.0;
};

struct BodyRateLimit
{
	double roll_rate_rad_s = 0.0;
	double pitch_rate_rad_s = 0.0;
	double yaw_rate_rad_s = 0.0;
};

struct LimitedBodyRateReference
{
	BodyRateReference value;
	bool constrained = false;
};

struct InnerRateLoopState
{
	double roll_integral = 0.0;
	double pitch_integral = 0.0;
	double yaw_integral = 0.0;
};

struct InnerRateLoopGains
{
	double roll_proportional = 0.0;
	double roll_integral = 0.0;
	double pitch_proportional = 0.0;
	double pitch_integral = 0.0;
	double yaw_proportional = 0.0;
	double yaw_integral = 0.0;
	double anti_windup = 0.0;
	double integral_limit = 0.0;
};

struct InnerRateLoopStepInput
{
	double dt_s = 0.0;
	BodyRateReference reference;
	BodyRateReference measured;
	InnerRateLoopGains gains;
};

struct InnerRateLoopResult
{
	InnerRateLoopState state;
	NormalizedControlSurfaceDemand surface_demand;
	BodyRateReference error;
	bool anti_windup_active = false;
};

LimitedBodyRateReference limit_body_rate_reference(
	const BodyRateReference& reference,
	const BodyRateLimit& limit);
InnerRateLoopResult update_inner_rate_loop(
	const InnerRateLoopState& current,
	const InnerRateLoopStepInput& input);
}
