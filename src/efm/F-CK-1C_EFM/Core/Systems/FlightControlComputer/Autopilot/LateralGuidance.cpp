#include "LateralGuidance.h"

#include "Common/Clamp.h"
#include "Common/Units.h"

#include <cmath>

namespace
{
constexpr double kMinimumNormalizedCommand = -1.0;
constexpr double kMaximumNormalizedCommand = 1.0;
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

double LateralGuidance::update(
	const AutomaticFlightControlObservation& observation,
	double target_heading_rad,
	bool active)
{
	if (!active)
	{
		return 0.0;
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
	const double bank_error = desired_bank - observation.roll_rad;
	const double command = config_.bank_kp * bank_error -
		config_.bank_kd * observation.legacy_heading_damping_rate_rad_s;
	return Common::limit(
		command, kMinimumNormalizedCommand, kMaximumNormalizedCommand);
}

void LateralGuidance::reset()
{
	heading_integral_ = 0.0;
}
}
}
