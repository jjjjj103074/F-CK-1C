#pragma once

// Private rate-loop implementation for FlightControlLaws.

namespace Systems
{
struct AxisRateLoopGains
{
	double proportional = 0.0;
	double integral = 0.0;
	double anti_windup = 0.0;
	double integral_limit = 0.0;
};

struct AxisRateLoopStepInput
{
	double dt_s = 0.0;
	double reference_rad_s = 0.0;
	double measured_rad_s = 0.0;
	AxisRateLoopGains gains;
};

struct AxisRateLoopResult
{
	double integral = 0.0;
	double effort_normalized = 0.0;
	double error_rad_s = 0.0;
	bool anti_windup_active = false;
};

AxisRateLoopResult update_axis_rate_loop(
	double current_integral,
	const AxisRateLoopStepInput& input);
}
