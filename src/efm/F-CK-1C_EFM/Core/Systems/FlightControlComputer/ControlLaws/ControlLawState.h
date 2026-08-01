#pragma once

#include "ControlLawConfig.h"

namespace Systems
{
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
};
}
