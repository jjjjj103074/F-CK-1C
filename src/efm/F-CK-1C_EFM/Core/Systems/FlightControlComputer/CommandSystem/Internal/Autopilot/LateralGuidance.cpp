#include "LateralGuidance.h"

// Private AFCS lateral-guidance implementation.

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
		bank_reference_rad_ = Common::limit(
			input.observation.roll_rad,
			-config_.bank_limit_rad,
			config_.bank_limit_rad);
		active_last_tick_ = true;
	}
	if (input.mode != previous_mode_)
	{
		heading_error_integral_deg_s_ = 0.0;
		previous_mode_ = input.mode;
	}
	double desired_bank_rad = input.target_roll_rad;
	if (input.mode == AutomaticFlightControlLateralMode::HeadingSelect)
		desired_bank_rad = heading_select_bank(input);
	return { rate_limit_bank(desired_bank_rad, input.observation) };
}

double LateralGuidance::heading_select_bank(
	const LateralGuidanceStepInput& input)
{
	const double heading_error_deg = Common::shortest_heading_difference_deg(
		input.target_heading_deg,
		input.observation.magnetic_heading_deg);
	if (!input.constrained)
	{
		heading_error_integral_deg_s_ = Common::limit(
			heading_error_integral_deg_s_ +
				heading_error_deg * input.observation.dt_s,
			-config_.heading_error_integral_limit_deg_s,
			config_.heading_error_integral_limit_deg_s);
	}
	return Common::limit(
		config_.heading_kp * heading_error_deg +
			config_.heading_ki * heading_error_integral_deg_s_,
		-config_.bank_limit_rad,
		config_.bank_limit_rad);
}

double LateralGuidance::rate_limit_bank(
	double desired_bank_rad,
	const AutomaticFlightControlObservation& observation)
{
	desired_bank_rad = Common::limit(
		desired_bank_rad, -config_.bank_limit_rad, config_.bank_limit_rad);
	const double maximum_step =
		config_.roll_reference_rate_rad_s * observation.dt_s;
	bank_reference_rad_ = Common::limit(
		desired_bank_rad,
		bank_reference_rad_ - maximum_step,
		bank_reference_rad_ + maximum_step);
	return bank_reference_rad_;
}

void LateralGuidance::track_observation(
	const AutomaticFlightControlObservation& observation)
{
	bank_reference_rad_ = Common::limit(
		observation.roll_rad, -config_.bank_limit_rad, config_.bank_limit_rad);
	heading_error_integral_deg_s_ = 0.0;
	active_last_tick_ = true;
}

void LateralGuidance::reset()
{
	heading_error_integral_deg_s_ = 0.0;
	active_last_tick_ = false;
	previous_mode_ = AutomaticFlightControlLateralMode::Off;
}
}
}
