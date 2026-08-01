#include "LateralGuidance.h"

#include "Common/Clamp.h"
#include "Common/Units.h"

#include <cmath>

namespace
{
constexpr double kHeadingIntegralLimit = 0.5;

double wrap_pi(double angle_rad)
{
	return std::atan2(std::sin(angle_rad), std::cos(angle_rad));
}
}

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
	const AutomaticFlightControlObservation& observation,
	double target_heading_rad,
	bool active)
{
	if (!active)
	{
		reset();
		return {};
	}
	if (!active_last_tick_)
	{
		bank_reference_rad_ = observation.roll_rad;
		active_last_tick_ = true;
	}
	const double heading_error =
		wrap_pi(target_heading_rad - observation.heading_rad);
	heading_integral_ = Common::limit(
		heading_integral_ + heading_error * observation.dt_s,
		-kHeadingIntegralLimit,
		kHeadingIntegralLimit);
	const double desired_bank = Common::limit(
		config_.heading_kp * heading_error +
			config_.heading_ki * heading_integral_,
		-config_.bank_limit_rad,
		config_.bank_limit_rad);
	const double maximum_step =
		config_.roll_reference_rate_rad_s * observation.dt_s;
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
