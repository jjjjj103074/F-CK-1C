#pragma once

#include "ControlLawConfig.h"

namespace Systems
{
struct FlightGuidanceEnvelope
{
	double bank_limit_rad = 0.0;
	double roll_rate_limit_rad_s = 0.0;
	double minimum_normal_acceleration_g = 0.0;
	double maximum_normal_acceleration_g = 0.0;
};

struct HardProtectionEnvelope
{
	double bank_limit_rad = 0.0;
	double minimum_normal_acceleration_g = 0.0;
	double maximum_normal_acceleration_g = 0.0;
	double angle_of_attack_limit_deg = 0.0;
	double roll_rate_limit_rad_s = 0.0;
	double pitch_rate_limit_rad_s = 0.0;
	double yaw_rate_limit_rad_s = 0.0;
};

struct ManeuverEnvelope
{
	FlightGuidanceEnvelope guidance;
	HardProtectionEnvelope hard_protection;
};

struct ManeuverEnvelopeInput
{
	FBWCatParams cat;
	double sensed_angle_of_attack_limit_deg = 0.0;
	bool developer_g_limiter_override_active = false;
};

ManeuverEnvelope make_maneuver_envelope(
	const FBWControllerConfig& config,
	const ManeuverEnvelopeInput& input);
void validate_maneuver_envelope(const ManeuverEnvelope& envelope);
}
