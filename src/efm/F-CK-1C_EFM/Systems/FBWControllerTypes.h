#pragma once

#include "../Common/Units.h"

namespace Systems
{
enum FBWCatMode
{
	FBW_CAT1 = 0,
	FBW_CAT3 = 1
};

enum FBWControlState
{
	FBW_STATE_RATE = 0,
	FBW_STATE_HOLD = 1,
	FBW_STATE_DEGRADE = 2
};

enum FBWHoldExitReason
{
	FBW_HOLD_EXIT_NONE = 0,
	FBW_HOLD_EXIT_STICK = 1,
	FBW_HOLD_EXIT_AOA = 2,
	FBW_HOLD_EXIT_QBAR = 3,
	FBW_HOLD_EXIT_ACTUATOR_SAT = 4,
	FBW_HOLD_EXIT_HOLD_CMD = 5
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
	double ail_limit_deg = 22.0;
	double ele_limit_deg = 25.0;
	double rud_limit_deg = 30.0;
	double ail_rate_deg_s = 110.0;
	double ele_rate_deg_s = 120.0;
	double rud_rate_deg_s = 80.0;
	double ail_lag_tau = 0.05;
	double ele_lag_tau = 0.04;
	double rud_lag_tau = 0.07;
};

struct FBWControllerState
{
	double throttle_cmd_left = 0.0;
	double throttle_cmd_right = 0.0;
	double throttle_blend = 0.0;
	bool throttle_override = false;
	bool g_limiter_override = false;
	bool enabled = true;
	FBWCatMode mode_target = FBW_CAT1;
	double mode_blend = 0.0;
	FBWControlState control_state = FBW_STATE_RATE;
	bool hold_active = false;
	FBWHoldExitReason hold_exit_reason = FBW_HOLD_EXIT_NONE;
	int hold_enter_reason = 0;
	double hold_timer = 0.0;
	double hold_gain_scale = 1.0;
	double phi_ref = 0.0;
	double theta_ref = 0.0;
	double int_p = 0.0;
	double int_q = 0.0;
	double int_r = 0.0;
	double ail_rate_state_deg = 0.0;
	double ail_lag_state_deg = 0.0;
	double ele_rate_state_deg = 0.0;
	double ele_lag_state_deg = 0.0;
	double rud_rate_state_deg = 0.0;
	double rud_lag_state_deg = 0.0;
	bool aoa_limit_active = false;
	bool rate_limit_active = false;
	bool actuator_sat = false;
	bool anti_windup_active = false;
	double actuator_sat_timer = 0.0;
	double stick_roll_raw = 0.0;
	double stick_pitch_raw = 0.0;
	double stick_yaw_raw = 0.0;
	double stick_roll_shaped = 0.0;
	double stick_pitch_shaped = 0.0;
	double stick_yaw_shaped = 0.0;
	double p_cmd = 0.0;
	double q_cmd = 0.0;
	double r_cmd = 0.0;
	double p_cmd_rate = 0.0;
	double q_cmd_rate = 0.0;
	double r_cmd_rate = 0.0;
	double p_cmd_hold = 0.0;
	double q_cmd_hold = 0.0;
	double r_cmd_damper = 0.0;
	double p_err = 0.0;
	double q_err = 0.0;
	double r_err = 0.0;
	double phi_err = 0.0;
	double theta_err = 0.0;
	double phi_raw = 0.0;
	double theta_raw = 0.0;
	double p_raw = 0.0;
	double q_raw = 0.0;
	double r_raw = 0.0;
	double alpha_raw = 0.0;
	double beta_raw = 0.0;
	double qbar_raw = 0.0;
	double ias_raw = 0.0;
	double mach_raw = 0.0;
	double phi_f = 0.0;
	double theta_f = 0.0;
	double p_f = 0.0;
	double q_f = 0.0;
	double r_f = 0.0;
	double alpha_f = 0.0;
	double beta_f = 0.0;
	double qbar_f = 0.0;
	double ias_f = 0.0;
	double mach_f = 0.0;
	double nz_raw = 1.0;
	double nz_f = 1.0;
	double alpha_trim_deg = 0.0;
	double nz_trim_g = 1.0;
	double alpha_outer_int = 0.0;
	double nz_outer_int = 0.0;
	double w_alpha = 1.0;
	double w_nz = 0.0;
	double w_q = 0.0;
	double alpha_cmd_deg = 0.0;
	double alpha_cmd_lim_deg = 0.0;
	double nz_cmd = 1.0;
	double nz_cmd_lim = 1.0;
	double q_cmd_direct = 0.0;
	double q_ref_alpha = 0.0;
	double q_ref_nz = 0.0;
	double q_ref_q = 0.0;
	double q_ref_blended = 0.0;
	double q_ref_filtered = 0.0;
	bool g_limit_active = false;
	double ail_cmd_pre = 0.0;
	double ail_cmd_sat = 0.0;
	double ail_cmd_rate = 0.0;
	double ail_cmd_lag = 0.0;
	double ele_cmd_pre = 0.0;
	double ele_cmd_sat = 0.0;
	double ele_cmd_rate = 0.0;
	double ele_cmd_lag = 0.0;
	double rud_cmd_pre = 0.0;
	double rud_cmd_sat = 0.0;
	double rud_cmd_rate = 0.0;
	double rud_cmd_lag = 0.0;
};

struct FBWControllerInput
{
	double dt = 0.0;
	double qbar = 0.0;
	double alpha_limit_deg = 0.0;
	double roll = 0.0;
	double pitch = 0.0;
	double roll_rate = 0.0;
	double pitch_rate = 0.0;
	double yaw_rate = 0.0;
	double alpha = 0.0;
	double beta = 0.0;
	double speed_scalar = 0.0;
	double mach = 0.0;
	double g = 1.0;
	double roll_input = 0.0;
	double roll_trim = 0.0;
	double pitch_input = 0.0;
	double pitch_trim = 0.0;
	double yaw_input = 0.0;
	double yaw_trim = 0.0;
	double gear_pos = 0.0;
	bool wow = false;
	double elevator_command = 0.0;
	double aileron_command = 0.0;
	double rudder_command = 0.0;
};

struct FBWControllerOutput
{
	double elevator_command = 0.0;
	double aileron_command = 0.0;
	double rudder_command = 0.0;
};

struct FBWResetInput
{
	double roll = 0.0;
	double pitch = 0.0;
	double alpha = 0.0;
	double g = 1.0;
};
}
