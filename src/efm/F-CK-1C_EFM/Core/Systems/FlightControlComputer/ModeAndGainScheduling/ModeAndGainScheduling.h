#pragma once

#include "../ControlLaws/ControlLawConfig.h"

// Public boundary for stores configuration, gain scheduling and envelopes.

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
	double angle_of_attack_blend_start_rad = 0.0;
	double angle_of_attack_limit_rad = 0.0;
	double roll_rate_limit_rad_s = 0.0;
	double pitch_rate_limit_rad_s = 0.0;
	double yaw_rate_limit_rad_s = 0.0;
};

struct ManeuverEnvelope
{
	FlightGuidanceEnvelope guidance;
	HardProtectionEnvelope hard_protection;
};

struct ActiveFlightControlConfiguration
{
	StoresConfiguration stores_configuration = StoresConfiguration::Cat1;
	double stores_transition_0_1 = 0.0;
	StoresControlLawSchedule stores;
	GainScheduleValues gains;
	ManeuverEnvelope envelope;
};

struct ModeAndGainSchedulingStepInput
{
	double dt_s = 0.0;
	double dynamic_pressure_pa = 0.0;
	double mach = 0.0;
	bool update_slow_gain_schedule = false;
	bool developer_g_limiter_override_active = false;
	double developer_g_limiter_override_margin_g = 0.0;
	bool landing_gear_handle_down = false;
};

struct ManeuverEnvelopeInput
{
	const ModeAndGainSchedulingConfig& config;
	const StoresControlLawSchedule& stores;
	double mach = 0.0;
	bool developer_g_limiter_override_active = false;
	double developer_g_limiter_override_margin_g = 0.0;
	bool landing_gear_handle_down = false;
};

class ModeAndGainScheduling
{
public:
	explicit ModeAndGainScheduling(
		const ModeAndGainSchedulingConfig& config);
	const ActiveFlightControlConfiguration& update(
		const ModeAndGainSchedulingStepInput& input);
	void set_stores_configuration(StoresConfiguration configuration);
	void toggle_stores_configuration(bool command_pressed);
	StoresConfiguration target_stores_configuration() const;
	double transition_0_1() const;

private:
	const ModeAndGainSchedulingConfig config_;
	StoresConfiguration target_ = StoresConfiguration::Cat1;
	ActiveFlightControlConfiguration active_;
};

ManeuverEnvelope make_maneuver_envelope(
	const ManeuverEnvelopeInput& input);
void validate_maneuver_envelope(const ManeuverEnvelope& envelope);
}
