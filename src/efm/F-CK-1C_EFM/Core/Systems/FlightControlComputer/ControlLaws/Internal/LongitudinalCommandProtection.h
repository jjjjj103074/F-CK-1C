#pragma once

#include "../ControlLawConfig.h"

// FlightControlLaws implementation detail. Protection modifies the active
// longitudinal command before feedback synthesis; it never edits pilot input
// or clamps a control surface directly.

namespace Systems
{
struct AlphaProtectionSchedule
{
	double angle_of_attack_rad = 0.0;
	double blend_start_rad = 0.0;
	double limit_rad = 0.0;
};

struct NormalAccelerationProtectionInput
{
	double requested_g = 1.0;
	double positive_limit_g = 1.0;
	AlphaProtectionSchedule alpha;
	const LongitudinalControlConfig& config;
};

struct NormalAccelerationProtectionResult
{
	double protected_g = 1.0;
	double command_decrement_g = 0.0;
	double alpha_blend_0_1 = 0.0;
	double maximum_g = 1.0;
	bool command_limited = false;
};

struct PitchRateProtectionInput
{
	double requested_rad_s = 0.0;
	AlphaProtectionSchedule alpha;
};

struct PitchRateProtectionResult
{
	double protected_rad_s = 0.0;
	double alpha_blend_0_1 = 0.0;
	bool command_limited = false;
};

NormalAccelerationProtectionResult protect_normal_acceleration_command(
	const NormalAccelerationProtectionInput& input);
PitchRateProtectionResult protect_pitch_rate_command(
	const PitchRateProtectionInput& input);
}
