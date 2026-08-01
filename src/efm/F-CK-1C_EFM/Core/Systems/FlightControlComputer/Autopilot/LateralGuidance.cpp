#include "LateralGuidance.h"

#include "Common/Angles.h"
#include "Common/Clamp.h"

#include <cmath>

namespace Core
{
namespace Systems
{
LateralGuidance::LateralGuidance(
	const AutomaticFlightControlConfig& config)
	: config_(config)
{
}

LateralGuidanceReference LateralGuidance::update(
	const LateralGuidanceStepInput& input)
{
	if (!input.active)
	{
		reset();
		return {};
	}
	if (!active_last_tick_)
	{
		bank_reference_rad_ = input.observation.roll_rad;
		active_last_tick_ = true;
	}
	const double heading_error = Common::shortest_angle_difference_rad(
		input.target_heading_rad, input.observation.heading_rad);
	if (!input.constrained)
	{
		heading_integral_ = Common::limit(
			heading_integral_ + heading_error * input.observation.dt_s,
			-config_.heading_error_integral_limit_rad_s,
			config_.heading_error_integral_limit_rad_s);
	}
	const double desired_bank = Common::limit(
		config_.heading_kp * heading_error +
			config_.heading_ki * heading_integral_,
		-config_.bank_limit_rad,
		config_.bank_limit_rad);
	const double maximum_step =
		config_.roll_reference_rate_rad_s * input.observation.dt_s;
	bank_reference_rad_ = Common::limit(
		desired_bank,
		bank_reference_rad_ - maximum_step,
		bank_reference_rad_ + maximum_step);
	return { bank_reference_rad_ };
}

void LateralGuidance::reset()
{
	heading_integral_ = 0.0;
	active_last_tick_ = false;
}
}
}
