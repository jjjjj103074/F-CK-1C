#include "ConfigurationAndMode.h"

#include <cmath>
#include <stdexcept>

namespace
{
constexpr double kDeveloperGLimiterOverrideMarginG = 2.0;

bool finite(double value)
{
	return std::isfinite(value);
}
}

namespace Systems
{
ManeuverEnvelope make_maneuver_envelope(
	const FBWControllerConfig& config,
	const ManeuverEnvelopeInput& input)
{
	const double maximum_normal_acceleration_g = input.cat.g_hard +
		(input.developer_g_limiter_override_active
			? kDeveloperGLimiterOverrideMarginG : 0.0);
	ManeuverEnvelope result = {
		{
			config.guidance_bank_limit_rad,
			config.guidance_roll_rate_limit_rad_s,
			config.guidance_min_normal_acceleration_g,
			config.guidance_max_normal_acceleration_g
		},
		{
			config.hard_bank_limit_rad,
			config.hard_min_normal_acceleration_g,
			maximum_normal_acceleration_g,
			input.sensed_angle_of_attack_limit_deg,
			input.cat.p_rate_limit,
			input.cat.q_rate_limit,
			input.cat.r_rate_limit
		}
	};
	validate_maneuver_envelope(result);
	return result;
}

void validate_maneuver_envelope(const ManeuverEnvelope& envelope)
{
	const double values[] = {
		envelope.guidance.bank_limit_rad,
		envelope.guidance.roll_rate_limit_rad_s,
		envelope.guidance.minimum_normal_acceleration_g,
		envelope.guidance.maximum_normal_acceleration_g,
		envelope.hard_protection.bank_limit_rad,
		envelope.hard_protection.minimum_normal_acceleration_g,
		envelope.hard_protection.maximum_normal_acceleration_g,
		envelope.hard_protection.angle_of_attack_limit_deg,
		envelope.hard_protection.roll_rate_limit_rad_s,
		envelope.hard_protection.pitch_rate_limit_rad_s,
		envelope.hard_protection.yaw_rate_limit_rad_s
	};
	for (double value : values)
	{
		if (!finite(value))
			throw std::invalid_argument("Non-finite maneuver envelope.");
	}
	const bool guidance_inside_hard =
		envelope.guidance.bank_limit_rad <=
			envelope.hard_protection.bank_limit_rad &&
		envelope.guidance.roll_rate_limit_rad_s <=
			envelope.hard_protection.roll_rate_limit_rad_s &&
		envelope.guidance.minimum_normal_acceleration_g >=
			envelope.hard_protection.minimum_normal_acceleration_g &&
		envelope.guidance.maximum_normal_acceleration_g <=
			envelope.hard_protection.maximum_normal_acceleration_g;
	if (!guidance_inside_hard)
	{
		throw std::invalid_argument(
			"Flight-guidance envelope exceeds hard protection.");
	}
}
}
