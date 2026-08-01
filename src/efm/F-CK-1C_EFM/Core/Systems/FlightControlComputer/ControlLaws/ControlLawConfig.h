#pragma once

#include "Common/Units.h"

namespace Systems
{
enum FBWCatMode
{
	FBW_CAT1 = 0,
	FBW_CAT3 = 1
};

struct FBWCatParams
{
	double deadband = 0.0;
	double hold_engage_time = 0.0;
	double hold_phi_kp = 0.0;
	double hold_theta_kp = 0.0;
	double hold_p_cmd_max = 0.0;
	double hold_q_cmd_max = 0.0;
	double alpha_hold_degrade_deg = 0.0;
	double qbar_min_hold = 0.0;
	double sat_time = 0.0;
	double hold_cmd_ratio_limit = 0.0;
	double hold_decay_tau = 0.0;
	double command_shape_tau = 0.0;
	double command_shape_rate = 0.0;
	double stick_expo = 0.0;
	double p_cmd_max = 0.0;
	double q_cmd_max = 0.0;
	double r_cmd_max = 0.0;
	double aoa_soft_deg = 0.0;
	double aoa_hard_deg = 0.0;
	double g_soft = 0.0;
	double g_hard = 0.0;
	double p_rate_limit = 0.0;
	double q_rate_limit = 0.0;
	double r_rate_limit = 0.0;
	double yaw_damper_beta = 0.0;
	double yaw_damper_r = 0.0;
};

struct FBWGainSchedulePoint
{
	double qbar = 0.0;
	double cmd_gain = 0.0;
	double hold_gain = 0.0;
	double damping_gain = 0.0;
	double limiter_gain = 0.0;
};

struct FBWGainScheduleValues
{
	double cmd_gain = 0.0;
	double hold_gain = 0.0;
	double damping_gain = 0.0;
	double limiter_gain = 0.0;
};

constexpr unsigned kFBWGainScheduleSize = 4;

inline FBWCatParams make_fbw_cat1_params()
{
	FBWCatParams params;
	params.deadband = 0.03;
	params.hold_engage_time = 0.18;
	params.hold_phi_kp = 2.8;
	params.hold_theta_kp = 2.2;
	params.hold_p_cmd_max = Common::rad(55.0);
	params.hold_q_cmd_max = Common::rad(42.0);
	params.alpha_hold_degrade_deg = 15.5;
	params.qbar_min_hold = 2500.0;
	params.sat_time = 0.35;
	params.hold_cmd_ratio_limit = 0.85;
	params.hold_decay_tau = 0.65;
	params.command_shape_tau = 0.05;
	params.command_shape_rate = 9.5;
	params.stick_expo = 0.10;
	params.p_cmd_max = Common::rad(190.0);
	params.q_cmd_max = Common::rad(145.0);
	params.r_cmd_max = Common::rad(80.0);
	params.aoa_soft_deg = 15.0;
	params.aoa_hard_deg = 21.0;
	params.g_soft = 6.4;
	params.g_hard = 8.8;
	params.p_rate_limit = Common::rad(220.0);
	params.q_rate_limit = Common::rad(170.0);
	params.r_rate_limit = Common::rad(95.0);
	params.yaw_damper_beta = 0.90;
	params.yaw_damper_r = 0.60;
	return params;
}

inline FBWCatParams make_fbw_cat3_params()
{
	FBWCatParams params;
	params.deadband = 0.05;
	params.hold_engage_time = 0.26;
	params.hold_phi_kp = 2.0;
	params.hold_theta_kp = 1.6;
	params.hold_p_cmd_max = Common::rad(40.0);
	params.hold_q_cmd_max = Common::rad(30.0);
	params.alpha_hold_degrade_deg = 13.5;
	params.qbar_min_hold = 4000.0;
	params.sat_time = 0.22;
	params.hold_cmd_ratio_limit = 0.70;
	params.hold_decay_tau = 0.40;
	params.command_shape_tau = 0.10;
	params.command_shape_rate = 5.5;
	params.stick_expo = 0.20;
	params.p_cmd_max = Common::rad(140.0);
	params.q_cmd_max = Common::rad(110.0);
	params.r_cmd_max = Common::rad(60.0);
	params.aoa_soft_deg = 12.5;
	params.aoa_hard_deg = 17.5;
	params.g_soft = 5.8;
	params.g_hard = 7.6;
	params.p_rate_limit = Common::rad(170.0);
	params.q_rate_limit = Common::rad(130.0);
	params.r_rate_limit = Common::rad(75.0);
	params.yaw_damper_beta = 1.10;
	params.yaw_damper_r = 0.80;
	return params;
}

struct FBWControllerConfig
{
	// F-16 reference-derived AP guidance envelope.
	double guidance_bank_limit_rad = Common::rad(30.0);
	double guidance_roll_rate_limit_rad_s = Common::rad(20.0);
	double guidance_min_normal_acceleration_g = 0.5;
	double guidance_max_normal_acceleration_g = 2.0;
	// Project-defined outer bound; control-law CAT limits remain authoritative.
	double hard_bank_limit_rad = Common::rad(60.0);
	double hard_min_normal_acceleration_g = -2.5;
	double pitch_attitude_error_to_rate_gain = 2.5;
	double vertical_speed_error_to_acceleration_gain = 0.4;
	double bank_angle_error_to_roll_rate_gain = 2.0;
	double coordinated_turn_minimum_speed_mps = 30.0;
	FBWCatParams cat1 = make_fbw_cat1_params();
	FBWCatParams cat3 = make_fbw_cat3_params();
	FBWGainSchedulePoint gain_schedule[kFBWGainScheduleSize] = {
		{ 1500.0, 1.15, 1.20, 1.15, 0.82 },
		{ 5000.0, 1.05, 1.05, 1.00, 0.95 },
		{ 15000.0, 0.90, 0.85, 0.90, 1.00 },
		{ 35000.0, 0.75, 0.65, 0.80, 0.90 }
	};
	double mode_switch_tau = 0.45;
	double signal_filter_tau = 0.06;
	double qbar_filter_tau = 0.18;
	double kp_p = 0.55;
	double ki_p = 0.35;
	double kp_q = 0.88;
	double ki_q = 0.48;
	double kp_r = 0.65;
	double ki_r = 0.25;
	double aw_gain = 1.20;
	double int_limit = 1.20;
	double outer_aw_gain = 1.10;
	double outer_int_limit = Common::rad(75.0);
	double alpha_trim_tau = 1.20;
	double nz_trim_tau = 1.60;
	double nz_filter_tau = 0.26;
	double pitch_ref_tau = 0.14;
	double pitch_ref_rate_deg_s = 90.0;
	double nz_limit_gain_floor = 0.58;
	double nz_limit_buffer_bias = 0.15;
	double region_low_kts = 220.0;
	double region_high_kts = 380.0;
	double region_approach_kts = 240.0;
	double region_min_kts = 110.0;
	double region_alpha1_deg = 12.0;
	double region_alpha2_deg = 18.0;
	double alpha_cmd_per_stick_deg = 13.5;
	double q_cmd_land_max_deg = 50.0;
};
}
