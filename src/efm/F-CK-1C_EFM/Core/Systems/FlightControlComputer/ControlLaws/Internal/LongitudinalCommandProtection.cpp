#include "LongitudinalCommandProtection.h"

#include "Common/Clamp.h"

#include <cmath>

namespace
{
constexpr double kCommandDifferenceTolerance = 1.0e-9;

double alpha_blend(const Systems::AlphaProtectionSchedule& schedule)
{
	const double normalized = Common::limit(
		(schedule.angle_of_attack_rad - schedule.blend_start_rad) /
			(schedule.limit_rad - schedule.blend_start_rad),
		0.0,
		1.0);
	// Project-defined interpolation between reference-derived onset and limit.
	// It intentionally does not splice YF-16 slopes into F-16XL endpoints.
	return normalized * normalized * (3.0 - 2.0 * normalized);
}
}

namespace Systems
{
NormalAccelerationProtectionResult protect_normal_acceleration_command(
	const NormalAccelerationProtectionInput& input)
{
	const double blend = alpha_blend(input.alpha);
	const double terminal_g =
		input.config.angle_of_attack_limited_normal_acceleration_g;
	const double maximum_g = input.positive_limit_g +
		(terminal_g - input.positive_limit_g) * blend;
	const double protected_g = input.requested_g > maximum_g
		? maximum_g
		: input.requested_g;
	const double decrement_g = input.requested_g - protected_g;
	return {
		protected_g,
		decrement_g,
		blend,
		maximum_g,
		std::fabs(decrement_g) > kCommandDifferenceTolerance
	};
}

PitchRateProtectionResult protect_pitch_rate_command(
	const PitchRateProtectionInput& input)
{
	const double blend = alpha_blend(input.alpha);
	const double protected_rate = input.requested_rad_s > 0.0
		? input.requested_rad_s * (1.0 - blend)
		: input.requested_rad_s;
	return {
		protected_rate,
		blend,
		std::fabs(protected_rate - input.requested_rad_s) >
			kCommandDifferenceTolerance
	};
}
}
