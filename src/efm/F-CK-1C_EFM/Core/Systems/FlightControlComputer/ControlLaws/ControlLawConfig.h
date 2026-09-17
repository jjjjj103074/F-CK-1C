#pragma once

#include "Common/Units.h"

#include <array>
#include <vector>

namespace Systems
{
enum class StoresConfiguration
{
	Cat1,
	Cat3
};

struct PilotInputShapingConfig
{
	double deadband_normalized = 0.0;
	double command_time_constant_s = 0.0;
	double command_rate_normalized_s = 0.0;
	double cubic_weight = 0.0;
};

struct ManeuverEnvelopeSchedule
{
	double maximum_roll_command_rad_s = 0.0;
	double maximum_yaw_command_rad_s = 0.0;
	double soft_positive_load_factor_g = 0.0;
	double hard_positive_load_factor_g = 0.0;
	double roll_rate_limit_rad_s = 0.0;
	double pitch_rate_limit_rad_s = 0.0;
	double yaw_rate_limit_rad_s = 0.0;
};

struct DirectionalControlSchedule
{
	double sideslip_damping_s_inv = 0.0;
	double yaw_rate_damping = 0.0;
};

struct StoresControlLawSchedule
{
	PilotInputShapingConfig pilot_input;
	ManeuverEnvelopeSchedule envelope;
	DirectionalControlSchedule directional;
};

struct GainSchedulePoint
{
	double dynamic_pressure_pa = 0.0;
	double command_gain = 0.0;
	double damping_gain = 0.0;
	double limiter_gain = 0.0;
};

struct GainScheduleValues
{
	double command_gain = 1.0;
	double damping_gain = 1.0;
	double limiter_gain = 1.0;
};

struct InputSignalManagementConfig
{
	double signal_filter_time_constant_s = 0.06;
	double dynamic_pressure_filter_time_constant_s = 0.18;
	double normal_acceleration_filter_time_constant_s = 0.26;
};

inline StoresControlLawSchedule make_cat1_schedule()
{
	return {
		{ 0.03, 0.05, 9.5, 0.10 },
		{ Common::rad(190.0), Common::rad(80.0), 6.4, 8.8,
			Common::rad(220.0),
			Common::rad(170.0), Common::rad(95.0) },
		{ 0.90, 0.60 }
	};
}

inline StoresControlLawSchedule make_cat3_schedule()
{
	return {
		// CAT changes maneuver authority and gains, not the physical controller
		// center deadband. Keeping this equal to CAT I avoids a control step.
		{ 0.03, 0.10, 5.5, 0.20 },
		{ Common::rad(140.0), Common::rad(60.0), 5.8, 7.6,
			Common::rad(170.0),
			Common::rad(130.0), Common::rad(75.0) },
		{ 1.10, 0.80 }
	};
}

inline constexpr unsigned kGainScheduleSize = 4;

struct ModeAndGainSchedulingConfig
{
	StoresControlLawSchedule cat1 = make_cat1_schedule();
	StoresControlLawSchedule cat3 = make_cat3_schedule();
	std::array<GainSchedulePoint, kGainScheduleSize> gain_schedule = {{
		{ 1500.0, 1.15, 1.15, 0.82 },
		{ 5000.0, 1.05, 1.00, 0.95 },
		{ 15000.0, 0.90, 0.90, 1.00 },
		{ 35000.0, 0.75, 0.80, 0.90 }
	}};
	double stores_transition_time_constant_s = 0.45;
	double guidance_bank_limit_rad = Common::rad(30.0);
	double guidance_roll_rate_limit_rad_s = Common::rad(20.0);
	double guidance_minimum_load_factor_g = 0.5;
	double guidance_maximum_load_factor_g = 2.0;
	double hard_bank_limit_rad = Common::rad(60.0);
	double hard_minimum_load_factor_g = -2.5;
	// Reference-derived F-16XL endpoints. Mach interpolation remains
	// project-defined; landing selection follows the cockpit gear handle.
	std::vector<double> angle_of_attack_mach = { 0.0, 0.85, 0.95, 1.5 };
	std::vector<double> angle_of_attack_limit_rad = {
		Common::rad(29.0), Common::rad(29.0),
		Common::rad(26.0), Common::rad(26.0)
	};
	double cruise_angle_of_attack_blend_start_rad = Common::rad(19.0);
	double landing_angle_of_attack_blend_start_rad = Common::rad(10.0);
	double landing_angle_of_attack_limit_rad = Common::rad(16.0);
};

struct LongitudinalControlConfig
{
	double positive_buffer_minimum_g = 0.25;
	double negative_soft_minimum_g = 1.0;
	double negative_soft_ratio = 0.65;
	// Reference-derived topology; gains are project-defined for this EFM plant.
	double normal_acceleration_proportional_cat1 = 0.30;
	double normal_acceleration_proportional_cat3 = 0.22;
	double normal_acceleration_integral_cat1_s_inv = 0.16;
	double normal_acceleration_integral_cat3_s_inv = 0.10;
	double normal_acceleration_anti_windup_s_inv = 1.20;
	double normal_acceleration_integral_limit_effort = 0.65;
	double pitch_rate_washout_time_constant_s = 0.35;
	double pitch_rate_feedback_gain_s = 0.85;
	double angle_of_attack_stability_gain_rad_inv = 1.20;
	double pitch_rate_command_proportional_s = 0.88;
	double pitch_rate_command_integral_gain_rad_inv = 0.48;
	double pitch_rate_command_anti_windup_s_inv = 1.20;
	double pitch_rate_command_integral_limit_effort = 1.20;
	double angle_of_attack_limited_normal_acceleration_g = 1.0;
	double limit_buffer_bias_g = 0.15;
	double landing_pitch_rate_limit_rad_s = Common::rad(50.0);
};

struct InnerRateControlConfig
{
	double roll_proportional = 0.55;
	double roll_integral = 0.35;
	double yaw_proportional = 0.65;
	double yaw_integral = 0.25;
	double anti_windup_gain = 1.20;
	double integral_limit = 1.20;
};

struct SurfaceCommandMixerConfig
{
	double symmetric_stabilator_limit_rad = Common::rad(25.0);
	double differential_flaperon_limit_rad = Common::rad(22.0);
	double rudder_limit_rad = Common::rad(30.0);
};

struct FlightControlOutputConfig
{
	double selection_transition_time_s = 0.15;
	double tracking_tolerance_rad = Common::rad(2.0);
};

struct FlightControlDiagnosticsConfig
{
	// Project-defined diagnostic qualification; this does not alter the law.
	double control_authority_persistence_s = 0.5;
	double minimum_alpha_recovery_rate_rad_s = Common::rad(0.5);
};

struct FlightControlLawsConfig
{
	LongitudinalControlConfig longitudinal;
	InnerRateControlConfig inner_rate;
	SurfaceCommandMixerConfig surface_mixer;
};

struct GuidanceCoordinationConfig
{
	double pitch_error_to_rate_gain_s_inv = 2.5;
	double vertical_speed_error_to_acceleration_gain_s_inv = 0.25;
	double bank_error_to_roll_rate_gain_s_inv = 2.0;
	double coordinated_turn_minimum_speed_mps = 30.0;
};

struct FlightControlDevelopmentConfig
{
	bool direct_control_law = false;
	bool g_limiter_override_available = false;
	bool experimental_auto_throttle_available = false;
	double g_limiter_override_margin_g = 2.0;
};
}
