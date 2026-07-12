#pragma once

#include "../Common/Actuator.h"
#include "../Common/Clamp.h"
#include "../Common/Units.h"
#include <cmath>

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

static const unsigned kFBWGainScheduleSize = 4;

struct FBWControllerConfig
{
	FBWCatParams cat1 = {
		0.03, 0.18, 2.8, 2.2, Common::rad(55.0), Common::rad(42.0), 15.5, 2500.0, 0.35, 0.85, 0.65, 0.05, 9.5, 0.10,
		Common::rad(190.0), Common::rad(145.0), Common::rad(80.0), 15.0, 21.0, 6.4, 8.8, Common::rad(220.0), Common::rad(170.0), Common::rad(95.0), 0.90, 0.60
	};

	FBWCatParams cat3 = {
		0.05, 0.26, 2.0, 1.6, Common::rad(40.0), Common::rad(30.0), 13.5, 4000.0, 0.22, 0.70, 0.40, 0.10, 5.5, 0.20,
		Common::rad(140.0), Common::rad(110.0), Common::rad(60.0), 12.5, 17.5, 5.8, 7.6, Common::rad(170.0), Common::rad(130.0), Common::rad(75.0), 1.10, 0.80
	};

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


inline double fbw_blend_value(double a, double b, double t)
{
	return a + (b - a) * Common::limit(t, 0.0, 1.0);
}


inline double fbw_min(double a, double b)
{
	return (a < b) ? a : b;
}

inline double fbw_max(double a, double b)
{
	return (a > b) ? a : b;
}

inline double fbw_first_order(double current, double target, double tau, double dt)
{
	if (tau <= 1e-6)
	{
		return target;
	}
	const double k = Common::limit(dt / (tau + dt), 0.0, 1.0);
	return current + (target - current) * k;
}

inline double fbw_wrap_pi(double angle)
{
	while (angle > Common::kPi)
	{
		angle -= 2.0 * Common::kPi;
	}
	while (angle < -Common::kPi)
	{
		angle += 2.0 * Common::kPi;
	}
	return angle;
}

inline double fbw_soft_limit_symmetric(double command, double limit_value)
{
	if (limit_value <= 1e-6)
	{
		return 0.0;
	}
	return limit_value * std::tanh(command / limit_value);
}

inline double fbw_soft_limit_asymmetric(double command, double pos_limit, double neg_limit)
{
	if (command >= 0.0)
	{
		return fbw_soft_limit_symmetric(command, pos_limit);
	}
	return -fbw_soft_limit_symmetric(-command, neg_limit);
}

inline double fbw_smoothstep01(double t)
{
	t = Common::limit(t, 0.0, 1.0);
	return t * t * (3.0 - 2.0 * t);
}

inline double fbw_soft_clip_positive(double value, double soft_limit, double hard_limit)
{
	if (value <= soft_limit)
	{
		return value;
	}
	if (value >= hard_limit)
	{
		return hard_limit;
	}
	const double t = fbw_smoothstep01((value - soft_limit) / (hard_limit - soft_limit));
	return soft_limit + (hard_limit - soft_limit) * t;
}

inline double fbw_soft_clip_negative(double value, double soft_limit, double hard_limit)
{
	if (value >= soft_limit)
	{
		return value;
	}
	if (value <= hard_limit)
	{
		return hard_limit;
	}
	const double t = fbw_smoothstep01((soft_limit - value) / (soft_limit - hard_limit));
	return soft_limit - (soft_limit - hard_limit) * t;
}

inline FBWCatParams fbw_blend_cat_params(const FBWCatParams& cat1, const FBWCatParams& cat3, double t)
{
	FBWCatParams out;
	out.deadband = fbw_blend_value(cat1.deadband, cat3.deadband, t);
	out.hold_engage_time = fbw_blend_value(cat1.hold_engage_time, cat3.hold_engage_time, t);
	out.hold_phi_kp = fbw_blend_value(cat1.hold_phi_kp, cat3.hold_phi_kp, t);
	out.hold_theta_kp = fbw_blend_value(cat1.hold_theta_kp, cat3.hold_theta_kp, t);
	out.hold_p_cmd_max = fbw_blend_value(cat1.hold_p_cmd_max, cat3.hold_p_cmd_max, t);
	out.hold_q_cmd_max = fbw_blend_value(cat1.hold_q_cmd_max, cat3.hold_q_cmd_max, t);
	out.alpha_hold_degrade_deg = fbw_blend_value(cat1.alpha_hold_degrade_deg, cat3.alpha_hold_degrade_deg, t);
	out.qbar_min_hold = fbw_blend_value(cat1.qbar_min_hold, cat3.qbar_min_hold, t);
	out.sat_time = fbw_blend_value(cat1.sat_time, cat3.sat_time, t);
	out.hold_cmd_ratio_limit = fbw_blend_value(cat1.hold_cmd_ratio_limit, cat3.hold_cmd_ratio_limit, t);
	out.hold_decay_tau = fbw_blend_value(cat1.hold_decay_tau, cat3.hold_decay_tau, t);
	out.command_shape_tau = fbw_blend_value(cat1.command_shape_tau, cat3.command_shape_tau, t);
	out.command_shape_rate = fbw_blend_value(cat1.command_shape_rate, cat3.command_shape_rate, t);
	out.stick_expo = fbw_blend_value(cat1.stick_expo, cat3.stick_expo, t);
	out.p_cmd_max = fbw_blend_value(cat1.p_cmd_max, cat3.p_cmd_max, t);
	out.q_cmd_max = fbw_blend_value(cat1.q_cmd_max, cat3.q_cmd_max, t);
	out.r_cmd_max = fbw_blend_value(cat1.r_cmd_max, cat3.r_cmd_max, t);
	out.aoa_soft_deg = fbw_blend_value(cat1.aoa_soft_deg, cat3.aoa_soft_deg, t);
	out.aoa_hard_deg = fbw_blend_value(cat1.aoa_hard_deg, cat3.aoa_hard_deg, t);
	out.g_soft = fbw_blend_value(cat1.g_soft, cat3.g_soft, t);
	out.g_hard = fbw_blend_value(cat1.g_hard, cat3.g_hard, t);
	out.p_rate_limit = fbw_blend_value(cat1.p_rate_limit, cat3.p_rate_limit, t);
	out.q_rate_limit = fbw_blend_value(cat1.q_rate_limit, cat3.q_rate_limit, t);
	out.r_rate_limit = fbw_blend_value(cat1.r_rate_limit, cat3.r_rate_limit, t);
	out.yaw_damper_beta = fbw_blend_value(cat1.yaw_damper_beta, cat3.yaw_damper_beta, t);
	out.yaw_damper_r = fbw_blend_value(cat1.yaw_damper_r, cat3.yaw_damper_r, t);
	return out;
}

inline FBWGainScheduleValues fbw_eval_gain_schedule(
	const FBWControllerConfig& config,
	double qbar_value)
{
	const FBWGainSchedulePoint* gain_schedule = config.gain_schedule;
	FBWGainScheduleValues out;
	out.cmd_gain = gain_schedule[0].cmd_gain;
	out.hold_gain = gain_schedule[0].hold_gain;
	out.damping_gain = gain_schedule[0].damping_gain;
	out.limiter_gain = gain_schedule[0].limiter_gain;

	if (qbar_value <= gain_schedule[0].qbar)
	{
		return out;
	}

	for (unsigned i = 1; i < kFBWGainScheduleSize; ++i)
	{
		if (qbar_value <= gain_schedule[i].qbar)
		{
			const double q0 = gain_schedule[i - 1].qbar;
			const double q1 = gain_schedule[i].qbar;
			const double t = Common::limit((qbar_value - q0) / (q1 - q0), 0.0, 1.0);
			out.cmd_gain = fbw_blend_value(gain_schedule[i - 1].cmd_gain, gain_schedule[i].cmd_gain, t);
			out.hold_gain = fbw_blend_value(gain_schedule[i - 1].hold_gain, gain_schedule[i].hold_gain, t);
			out.damping_gain = fbw_blend_value(gain_schedule[i - 1].damping_gain, gain_schedule[i].damping_gain, t);
			out.limiter_gain = fbw_blend_value(gain_schedule[i - 1].limiter_gain, gain_schedule[i].limiter_gain, t);
			return out;
		}
	}

	out.cmd_gain = gain_schedule[kFBWGainScheduleSize - 1].cmd_gain;
	out.hold_gain = gain_schedule[kFBWGainScheduleSize - 1].hold_gain;
	out.damping_gain = gain_schedule[kFBWGainScheduleSize - 1].damping_gain;
	out.limiter_gain = gain_schedule[kFBWGainScheduleSize - 1].limiter_gain;
	return out;
}

inline const char* fbw_mode_name(const FBWControllerState& state)
{
	return (state.mode_blend >= 0.5) ? "CAT3" : "CAT1";
}

inline const char* fbw_state_name(const FBWControllerState& state)
{
	switch (state.control_state)
	{
	case FBW_STATE_HOLD:
		return "HOLD";
	case FBW_STATE_DEGRADE:
		return "DEGRADE";
	default:
		return "RATE";
	}
}

inline const char* fbw_exit_reason_name(const FBWControllerState& state)
{
	switch (state.hold_exit_reason)
	{
	case FBW_HOLD_EXIT_STICK:
		return "STICK";
	case FBW_HOLD_EXIT_AOA:
		return "AOA";
	case FBW_HOLD_EXIT_QBAR:
		return "QBAR";
	case FBW_HOLD_EXIT_ACTUATOR_SAT:
		return "SAT";
	case FBW_HOLD_EXIT_HOLD_CMD:
		return "HCMD";
	default:
		return "NONE";
	}
}

inline void reset_fbw_state(
	FBWControllerState& state,
	double roll,
	double pitch,
	double alpha,
	double g)
{
	state.mode_target = FBW_CAT1;
	state.mode_blend = 0.0;
	state.control_state = FBW_STATE_RATE;
	state.hold_active = false;
	state.hold_exit_reason = FBW_HOLD_EXIT_NONE;
	state.hold_enter_reason = 0;
	state.hold_timer = 0.0;
	state.hold_gain_scale = 1.0;
	state.phi_ref = roll;
	state.theta_ref = pitch;

	state.int_p = 0.0;
	state.int_q = 0.0;
	state.int_r = 0.0;

	state.ail_rate_state_deg = 0.0;
	state.ail_lag_state_deg = 0.0;
	state.ele_rate_state_deg = 0.0;
	state.ele_lag_state_deg = 0.0;
	state.rud_rate_state_deg = 0.0;
	state.rud_lag_state_deg = 0.0;

	state.stick_roll_shaped = 0.0;
	state.stick_pitch_shaped = 0.0;
	state.stick_yaw_shaped = 0.0;

	state.p_cmd = 0.0;
	state.q_cmd = 0.0;
	state.r_cmd = 0.0;
	state.p_cmd_rate = 0.0;
	state.q_cmd_rate = 0.0;
	state.r_cmd_rate = 0.0;
	state.p_cmd_hold = 0.0;
	state.q_cmd_hold = 0.0;
	state.r_cmd_damper = 0.0;
	state.p_err = 0.0;
	state.q_err = 0.0;
	state.r_err = 0.0;
	state.phi_err = 0.0;
	state.theta_err = 0.0;
	state.nz_raw = g;
	state.nz_f = g;
	state.alpha_trim_deg = alpha;
	state.nz_trim_g = g;
	state.alpha_outer_int = 0.0;
	state.nz_outer_int = 0.0;
	state.w_alpha = 1.0;
	state.w_nz = 0.0;
	state.w_q = 0.0;
	state.alpha_cmd_deg = alpha;
	state.alpha_cmd_lim_deg = alpha;
	state.nz_cmd = g;
	state.nz_cmd_lim = g;
	state.q_cmd_direct = 0.0;
	state.q_ref_alpha = 0.0;
	state.q_ref_nz = 0.0;
	state.q_ref_q = 0.0;
	state.q_ref_blended = 0.0;
	state.q_ref_filtered = 0.0;

	state.aoa_limit_active = false;
	state.rate_limit_active = false;
	state.actuator_sat = false;
	state.anti_windup_active = false;
	state.g_limit_active = false;
	state.actuator_sat_timer = 0.0;
}

inline double fbw_apply_axis_actuator(
	double cmd_norm,
	double limit_deg,
	double rate_deg_s,
	double lag_tau,
	double dt,
	double& rate_state_deg,
	double& lag_state_deg,
	double& dbg_pre_deg,
	double& dbg_sat_deg,
	double& dbg_rate_deg,
	double& dbg_lag_deg,
	bool& axis_saturated)
{
	dbg_pre_deg = cmd_norm * limit_deg;
	dbg_sat_deg = Common::limit(dbg_pre_deg, -limit_deg, limit_deg);

	const double max_step = rate_deg_s * dt;
	rate_state_deg = rate_state_deg + Common::limit(dbg_sat_deg - rate_state_deg, -max_step, max_step);
	rate_state_deg = Common::limit(rate_state_deg, -limit_deg, limit_deg);

	const double lag_alpha = Common::limit(dt / (lag_tau + dt), 0.0, 1.0);
	lag_state_deg = lag_state_deg + (rate_state_deg - lag_state_deg) * lag_alpha;
	lag_state_deg = Common::limit(lag_state_deg, -limit_deg, limit_deg);

	dbg_rate_deg = rate_state_deg;
	dbg_lag_deg = lag_state_deg;

	axis_saturated =
		(std::fabs(dbg_pre_deg - dbg_sat_deg) > 1e-3) ||
		(std::fabs(dbg_sat_deg - rate_state_deg) > 1e-3) ||
		(std::fabs(lag_state_deg) > (limit_deg - 1e-3));

	return Common::limit(lag_state_deg / limit_deg, -1.0, 1.0);
}


inline FBWControllerOutput update_fbw_controller(
	FBWControllerState& state,
	const FBWControllerConfig& config,
	const FBWControllerInput& input)
{
	FBWControllerOutput output;
	output.elevator_command = input.elevator_command;
	output.aileron_command = input.aileron_command;
	output.rudder_command = input.rudder_command;

	double& elevator_command = output.elevator_command;
	double& aileron_command = output.aileron_command;
	double& rudder_command = output.rudder_command;

	const double dt = input.dt;
	const double qbar = input.qbar;
	const double alpha_limit_deg = input.alpha_limit_deg;
	const double roll = input.roll;
	const double pitch = input.pitch;
	const double roll_rate = input.roll_rate;
	const double pitch_rate = input.pitch_rate;
	const double yaw_rate = input.yaw_rate;
	const double alpha = input.alpha;
	const double beta = input.beta;
	const double V_scalar = input.speed_scalar;
	const double mach = input.mach;
	const double g = input.g;
	const double roll_input = input.roll_input;
	const double roll_trim = input.roll_trim;
	const double pitch_input = input.pitch_input;
	const double pitch_trim = input.pitch_trim;
	const double yaw_input = input.yaw_input;
	const double yaw_trim = input.yaw_trim;
	const double gear_pos = input.gear_pos;
	const double rad_to_deg = Common::kDegPerRad;

	const FBWCatParams& fbw_cat1 = config.cat1;
	const FBWCatParams& fbw_cat3 = config.cat3;
	const double& fbw_mode_switch_tau = config.mode_switch_tau;
	const double& fbw_signal_filter_tau = config.signal_filter_tau;
	const double& fbw_qbar_filter_tau = config.qbar_filter_tau;
	const double& fbw_kp_p = config.kp_p;
	const double& fbw_ki_p = config.ki_p;
	const double& fbw_kp_q = config.kp_q;
	const double& fbw_ki_q = config.ki_q;
	const double& fbw_kp_r = config.kp_r;
	const double& fbw_ki_r = config.ki_r;
	const double& fbw_aw_gain = config.aw_gain;
	const double& fbw_int_limit = config.int_limit;
	const double& fbw_outer_aw_gain = config.outer_aw_gain;
	const double& fbw_outer_int_limit = config.outer_int_limit;
	const double& fbw_alpha_trim_tau = config.alpha_trim_tau;
	const double& fbw_nz_trim_tau = config.nz_trim_tau;
	const double& fbw_nz_filter_tau = config.nz_filter_tau;
	const double& fbw_pitch_ref_tau = config.pitch_ref_tau;
	const double& fbw_pitch_ref_rate_deg_s = config.pitch_ref_rate_deg_s;
	const double& fbw_nz_limit_gain_floor = config.nz_limit_gain_floor;
	const double& fbw_nz_limit_buffer_bias = config.nz_limit_buffer_bias;
	const double& fbw_region_low_kts = config.region_low_kts;
	const double& fbw_region_high_kts = config.region_high_kts;
	const double& fbw_region_approach_kts = config.region_approach_kts;
	const double& fbw_region_min_kts = config.region_min_kts;
	const double& fbw_region_alpha1_deg = config.region_alpha1_deg;
	const double& fbw_region_alpha2_deg = config.region_alpha2_deg;
	const double& fbw_alpha_cmd_per_stick_deg = config.alpha_cmd_per_stick_deg;
	const double& fbw_q_cmd_land_max_deg = config.q_cmd_land_max_deg;
	const double& fbw_ail_limit_deg = config.ail_limit_deg;
	const double& fbw_ele_limit_deg = config.ele_limit_deg;
	const double& fbw_rud_limit_deg = config.rud_limit_deg;
	const double& fbw_ail_rate_deg_s = config.ail_rate_deg_s;
	const double& fbw_ele_rate_deg_s = config.ele_rate_deg_s;
	const double& fbw_rud_rate_deg_s = config.rud_rate_deg_s;
	const double& fbw_ail_lag_tau = config.ail_lag_tau;
	const double& fbw_ele_lag_tau = config.ele_lag_tau;
	const double& fbw_rud_lag_tau = config.rud_lag_tau;

	bool& fbw_enabled = state.enabled;
	FBWCatMode& fbw_mode_target = state.mode_target;
	double& fbw_mode_blend = state.mode_blend;
	FBWControlState& fbw_state = state.control_state;
	bool& fbw_hold_active = state.hold_active;
	FBWHoldExitReason& fbw_hold_exit_reason = state.hold_exit_reason;
	int& fbw_hold_enter_reason = state.hold_enter_reason;
	double& fbw_hold_timer = state.hold_timer;
	double& fbw_hold_gain_scale = state.hold_gain_scale;
	double& fbw_phi_ref = state.phi_ref;
	double& fbw_theta_ref = state.theta_ref;
	double& fbw_int_p = state.int_p;
	double& fbw_int_q = state.int_q;
	double& fbw_int_r = state.int_r;
	double& fbw_ail_rate_state_deg = state.ail_rate_state_deg;
	double& fbw_ail_lag_state_deg = state.ail_lag_state_deg;
	double& fbw_ele_rate_state_deg = state.ele_rate_state_deg;
	double& fbw_ele_lag_state_deg = state.ele_lag_state_deg;
	double& fbw_rud_rate_state_deg = state.rud_rate_state_deg;
	double& fbw_rud_lag_state_deg = state.rud_lag_state_deg;
	bool& fbw_aoa_limit_active = state.aoa_limit_active;
	bool& fbw_rate_limit_active = state.rate_limit_active;
	bool& fbw_actuator_sat = state.actuator_sat;
	bool& fbw_anti_windup_active = state.anti_windup_active;
	double& fbw_actuator_sat_timer = state.actuator_sat_timer;
	double& fbw_stick_roll_raw = state.stick_roll_raw;
	double& fbw_stick_pitch_raw = state.stick_pitch_raw;
	double& fbw_stick_yaw_raw = state.stick_yaw_raw;
	double& fbw_stick_roll_shaped = state.stick_roll_shaped;
	double& fbw_stick_pitch_shaped = state.stick_pitch_shaped;
	double& fbw_stick_yaw_shaped = state.stick_yaw_shaped;
	double& fbw_p_cmd = state.p_cmd;
	double& fbw_q_cmd = state.q_cmd;
	double& fbw_r_cmd = state.r_cmd;
	double& fbw_p_cmd_rate = state.p_cmd_rate;
	double& fbw_q_cmd_rate = state.q_cmd_rate;
	double& fbw_r_cmd_rate = state.r_cmd_rate;
	double& fbw_p_cmd_hold = state.p_cmd_hold;
	double& fbw_q_cmd_hold = state.q_cmd_hold;
	double& fbw_r_cmd_damper = state.r_cmd_damper;
	double& fbw_p_err = state.p_err;
	double& fbw_q_err = state.q_err;
	double& fbw_r_err = state.r_err;
	double& fbw_phi_err = state.phi_err;
	double& fbw_theta_err = state.theta_err;
	double& fbw_phi_raw = state.phi_raw;
	double& fbw_theta_raw = state.theta_raw;
	double& fbw_p_raw = state.p_raw;
	double& fbw_q_raw = state.q_raw;
	double& fbw_r_raw = state.r_raw;
	double& fbw_alpha_raw = state.alpha_raw;
	double& fbw_beta_raw = state.beta_raw;
	double& fbw_qbar_raw = state.qbar_raw;
	double& fbw_ias_raw = state.ias_raw;
	double& fbw_mach_raw = state.mach_raw;
	double& fbw_phi_f = state.phi_f;
	double& fbw_theta_f = state.theta_f;
	double& fbw_p_f = state.p_f;
	double& fbw_q_f = state.q_f;
	double& fbw_r_f = state.r_f;
	double& fbw_alpha_f = state.alpha_f;
	double& fbw_beta_f = state.beta_f;
	double& fbw_qbar_f = state.qbar_f;
	double& fbw_ias_f = state.ias_f;
	double& fbw_mach_f = state.mach_f;
	double& fbw_nz_raw = state.nz_raw;
	double& fbw_nz_f = state.nz_f;
	double& fbw_alpha_trim_deg = state.alpha_trim_deg;
	double& fbw_nz_trim_g = state.nz_trim_g;
	double& fbw_alpha_outer_int = state.alpha_outer_int;
	double& fbw_nz_outer_int = state.nz_outer_int;
	double& fbw_w_alpha = state.w_alpha;
	double& fbw_w_nz = state.w_nz;
	double& fbw_w_q = state.w_q;
	double& fbw_alpha_cmd_deg = state.alpha_cmd_deg;
	double& fbw_alpha_cmd_lim_deg = state.alpha_cmd_lim_deg;
	double& fbw_nz_cmd = state.nz_cmd;
	double& fbw_nz_cmd_lim = state.nz_cmd_lim;
	double& fbw_q_cmd_direct = state.q_cmd_direct;
	double& fbw_q_ref_alpha = state.q_ref_alpha;
	double& fbw_q_ref_nz = state.q_ref_nz;
	double& fbw_q_ref_q = state.q_ref_q;
	double& fbw_q_ref_blended = state.q_ref_blended;
	double& fbw_q_ref_filtered = state.q_ref_filtered;
	bool& fbw_g_limit_active = state.g_limit_active;
	double& fbw_ail_cmd_pre = state.ail_cmd_pre;
	double& fbw_ail_cmd_sat = state.ail_cmd_sat;
	double& fbw_ail_cmd_rate = state.ail_cmd_rate;
	double& fbw_ail_cmd_lag = state.ail_cmd_lag;
	double& fbw_ele_cmd_pre = state.ele_cmd_pre;
	double& fbw_ele_cmd_sat = state.ele_cmd_sat;
	double& fbw_ele_cmd_rate = state.ele_cmd_rate;
	double& fbw_ele_cmd_lag = state.ele_cmd_lag;
	double& fbw_rud_cmd_pre = state.rud_cmd_pre;
	double& fbw_rud_cmd_sat = state.rud_cmd_sat;
	double& fbw_rud_cmd_rate = state.rud_cmd_rate;
	double& fbw_rud_cmd_lag = state.rud_cmd_lag;
	bool& fbw_g_limiter_override = state.g_limiter_override;
	// RCAH state machine:
	// RATE    : non-zero stick -> rate command (p/q/r tracking)
	// HOLD    : stick stays in deadband for T_hold_engage -> lock phi/theta and hold attitude
	// DEGRADE : if near-limit/saturation/low-qbar, hold_gain_scale decays to 0 and falls back to damping
	// CAT switching is parameter-blended (fbw_mode_blend) to avoid control-law jumps.
	if (fbw_enabled == false)
	{
		elevator_command = Common::limit(Common::actuator(elevator_command, pitch_input + pitch_trim, -0.0125, 0.0125), -1, 1);
		aileron_command = Common::limit(Common::actuator(aileron_command, roll_input + roll_trim, -0.02, 0.02), -1, 1);
		rudder_command = Common::limit(Common::actuator(rudder_command, yaw_input + yaw_trim, -0.012, 0.012), -1, 1);
		return output;
	}

	const double mode_target = (fbw_mode_target == FBW_CAT3) ? 1.0 : 0.0;
	fbw_mode_blend = fbw_first_order(fbw_mode_blend, mode_target, fbw_mode_switch_tau, dt);

	const FBWCatParams cat = fbw_blend_cat_params(fbw_cat1, fbw_cat3, fbw_mode_blend);

	fbw_phi_raw = roll;
	fbw_theta_raw = pitch;
	fbw_p_raw = roll_rate;
	fbw_q_raw = pitch_rate;
	fbw_r_raw = yaw_rate;
	fbw_alpha_raw = alpha;
	fbw_beta_raw = beta;
	fbw_qbar_raw = qbar;
	fbw_ias_raw = V_scalar * 1.943844;
	fbw_mach_raw = mach;

	fbw_phi_f = fbw_first_order(fbw_phi_f, fbw_phi_raw, fbw_signal_filter_tau, dt);
	fbw_theta_f = fbw_first_order(fbw_theta_f, fbw_theta_raw, fbw_signal_filter_tau, dt);
	fbw_p_f = fbw_first_order(fbw_p_f, fbw_p_raw, fbw_signal_filter_tau, dt);
	fbw_q_f = fbw_first_order(fbw_q_f, fbw_q_raw, fbw_signal_filter_tau, dt);
	fbw_r_f = fbw_first_order(fbw_r_f, fbw_r_raw, fbw_signal_filter_tau, dt);
	fbw_alpha_f = fbw_first_order(fbw_alpha_f, fbw_alpha_raw, fbw_signal_filter_tau, dt);
	fbw_beta_f = fbw_first_order(fbw_beta_f, fbw_beta_raw, fbw_signal_filter_tau, dt);
	fbw_qbar_f = fbw_first_order(fbw_qbar_f, fbw_qbar_raw, fbw_qbar_filter_tau, dt);
	fbw_ias_f = fbw_first_order(fbw_ias_f, fbw_ias_raw, fbw_signal_filter_tau, dt);
	fbw_mach_f = fbw_first_order(fbw_mach_f, fbw_mach_raw, fbw_signal_filter_tau, dt);

	fbw_stick_roll_raw = Common::limit(roll_input + roll_trim, -1.0, 1.0);
	fbw_stick_pitch_raw = Common::limit(pitch_input + pitch_trim, -1.0, 1.0);
	fbw_stick_yaw_raw = Common::limit(yaw_input + yaw_trim, -1.0, 1.0);

	const double roll_target = (1.0 - cat.stick_expo) * fbw_stick_roll_raw + cat.stick_expo * fbw_stick_roll_raw * fbw_stick_roll_raw * fbw_stick_roll_raw;
	const double pitch_target = (1.0 - cat.stick_expo) * fbw_stick_pitch_raw + cat.stick_expo * fbw_stick_pitch_raw * fbw_stick_pitch_raw * fbw_stick_pitch_raw;
	const double yaw_target = (1.0 - cat.stick_expo) * fbw_stick_yaw_raw + cat.stick_expo * fbw_stick_yaw_raw * fbw_stick_yaw_raw * fbw_stick_yaw_raw;

	const double roll_prev = fbw_stick_roll_shaped;
	const double pitch_prev = fbw_stick_pitch_shaped;
	const double yaw_prev = fbw_stick_yaw_shaped;
	fbw_stick_roll_shaped = fbw_first_order(fbw_stick_roll_shaped, roll_target, cat.command_shape_tau, dt);
	fbw_stick_pitch_shaped = fbw_first_order(fbw_stick_pitch_shaped, pitch_target, cat.command_shape_tau, dt);
	fbw_stick_yaw_shaped = fbw_first_order(fbw_stick_yaw_shaped, yaw_target, cat.command_shape_tau, dt);
	const double shape_step = cat.command_shape_rate * dt;
	fbw_stick_roll_shaped = Common::limit(fbw_stick_roll_shaped, roll_prev - shape_step, roll_prev + shape_step);
	fbw_stick_pitch_shaped = Common::limit(fbw_stick_pitch_shaped, pitch_prev - shape_step, pitch_prev + shape_step);
	fbw_stick_yaw_shaped = Common::limit(fbw_stick_yaw_shaped, yaw_prev - shape_step, yaw_prev + shape_step);

	const bool stick_in_deadband = (std::fabs(fbw_stick_roll_raw) <= cat.deadband) && (std::fabs(fbw_stick_pitch_raw) <= cat.deadband);
	const bool wow = input.wow;
	if (wow)
	{
		fbw_state = FBW_STATE_RATE;
		fbw_hold_active = false;
		fbw_hold_timer = 0.0;
		fbw_hold_gain_scale = 1.0;
		fbw_hold_exit_reason = FBW_HOLD_EXIT_STICK;
		fbw_hold_enter_reason = 0;
	}
	else if (stick_in_deadband == false)
	{
		fbw_state = FBW_STATE_RATE;
		fbw_hold_active = false;
		fbw_hold_timer = 0.0;
		fbw_hold_gain_scale = 1.0;
		fbw_hold_exit_reason = FBW_HOLD_EXIT_STICK;
		fbw_hold_enter_reason = 0;
	}
	else
	{
		fbw_hold_timer += dt;
		if (fbw_state == FBW_STATE_RATE && fbw_hold_timer >= cat.hold_engage_time)
		{
			fbw_state = FBW_STATE_HOLD;
			fbw_hold_active = true;
			fbw_phi_ref = fbw_phi_f;
			fbw_theta_ref = fbw_theta_f;
			fbw_hold_gain_scale = 1.0;
			fbw_hold_exit_reason = FBW_HOLD_EXIT_NONE;
			fbw_hold_enter_reason = 1;
		}
	}

	const FBWGainScheduleValues gs = fbw_eval_gain_schedule(config, fbw_qbar_f);
	fbw_nz_raw = g;
	fbw_nz_f = fbw_first_order(fbw_nz_f, fbw_nz_raw, fbw_nz_filter_tau, dt);

	fbw_p_cmd_rate = fbw_stick_roll_shaped * cat.p_cmd_max * gs.cmd_gain;
	fbw_r_cmd_rate = fbw_stick_yaw_shaped * cat.r_cmd_max * gs.cmd_gain;

	const bool pitch_stick_in_deadband = std::fabs(fbw_stick_pitch_raw) <= cat.deadband;
	if (wow || pitch_stick_in_deadband)
	{
		fbw_alpha_trim_deg = fbw_first_order(fbw_alpha_trim_deg, fbw_alpha_f, fbw_alpha_trim_tau, dt);
		fbw_nz_trim_g = fbw_first_order(fbw_nz_trim_g, fbw_nz_f, fbw_nz_trim_tau, dt);
	}

	const double gear_weight = (gear_pos > 0.5) ? 1.0 : 0.0;
	const double approach_t = Common::limit((fbw_region_approach_kts - fbw_ias_f) / (fbw_region_approach_kts - fbw_region_min_kts), 0.0, 1.0);
	const double high_speed_t = Common::limit((fbw_ias_f - fbw_region_low_kts) / (fbw_region_high_kts - fbw_region_low_kts), 0.0, 1.0);
	const double alpha_region_t = Common::limit((std::fabs(fbw_alpha_f) - fbw_region_alpha1_deg) / (fbw_region_alpha2_deg - fbw_region_alpha1_deg), 0.0, 1.0);

	fbw_w_q = gear_weight * fbw_blend_value(0.35, 1.0, approach_t);
	fbw_w_nz = high_speed_t * (1.0 - fbw_w_q);
	fbw_w_alpha = 1.0 - fbw_w_nz - fbw_w_q;
	fbw_w_alpha = fbw_max(fbw_w_alpha, alpha_region_t);
	const double pitch_weight_sum = fbw_max(fbw_w_alpha + fbw_w_nz + fbw_w_q, 1e-6);
	fbw_w_alpha /= pitch_weight_sum;
	fbw_w_nz /= pitch_weight_sum;
	fbw_w_q /= pitch_weight_sum;

	fbw_phi_err = 0.0;
	fbw_theta_err = 0.0;
	fbw_p_cmd_hold = 0.0;
	fbw_q_cmd_hold = 0.0;
	bool hold_cmd_overlimit = false;
	if ((fbw_state == FBW_STATE_HOLD || fbw_state == FBW_STATE_DEGRADE) && stick_in_deadband)
	{
		fbw_phi_err = fbw_wrap_pi(fbw_phi_ref - fbw_phi_f);
		fbw_theta_err = fbw_theta_ref - fbw_theta_f;
		const double p_hold_raw = fbw_phi_err * cat.hold_phi_kp * gs.hold_gain;
		const double q_hold_raw = fbw_theta_err * cat.hold_theta_kp * gs.hold_gain;
		const double p_hold_lim = cat.hold_p_cmd_max * gs.limiter_gain;
		const double q_hold_lim = cat.hold_q_cmd_max * gs.limiter_gain;
		fbw_p_cmd_hold = Common::limit(p_hold_raw, -p_hold_lim, p_hold_lim);
		fbw_q_cmd_hold = Common::limit(q_hold_raw, -q_hold_lim, q_hold_lim);
		hold_cmd_overlimit =
			(std::fabs(p_hold_raw) > (p_hold_lim * cat.hold_cmd_ratio_limit)) ||
			(std::fabs(q_hold_raw) > (q_hold_lim * cat.hold_cmd_ratio_limit));
	}

	const double alpha_abs = std::fabs(fbw_alpha_f);
	const double alpha_soft = Common::limit(cat.aoa_soft_deg, 0.1, alpha_limit_deg);
	const double alpha_hard = Common::limit(cat.aoa_hard_deg, alpha_soft + 0.1, alpha_limit_deg + 5.0);
	const double alpha_cmd_range = fbw_blend_value(fbw_alpha_cmd_per_stick_deg, fbw_alpha_cmd_per_stick_deg * 0.65, high_speed_t);
	fbw_alpha_cmd_deg = fbw_alpha_trim_deg + fbw_stick_pitch_shaped * alpha_cmd_range;
	fbw_alpha_cmd_lim_deg = fbw_soft_limit_symmetric(fbw_alpha_cmd_deg, alpha_soft);

	const double nz_pos_limit = fbw_g_limiter_override ? (cat.g_hard + 2.0) : cat.g_hard;
	const double nz_neg_limit = 2.5;
	const double nz_pos_soft = fbw_min(cat.g_soft, nz_pos_limit - fbw_max(0.25, fbw_nz_limit_buffer_bias));
	const double nz_neg_hard = -nz_neg_limit;
	const double nz_neg_soft = -fbw_max(1.0, nz_neg_limit * 0.65);
	if (fbw_stick_pitch_shaped >= 0.0)
	{
		fbw_nz_cmd = 1.0 + fbw_stick_pitch_shaped * (nz_pos_limit - 1.0);
	}
	else
	{
		fbw_nz_cmd = 1.0 + fbw_stick_pitch_shaped * (1.0 + nz_neg_limit);
	}
	fbw_nz_cmd_lim = fbw_nz_cmd;
	if (fbw_nz_cmd_lim > nz_pos_soft)
	{
		fbw_nz_cmd_lim = fbw_soft_clip_positive(fbw_nz_cmd_lim, nz_pos_soft, nz_pos_limit);
	}
	if (fbw_nz_cmd_lim < nz_neg_soft)
	{
		fbw_nz_cmd_lim = fbw_soft_clip_negative(fbw_nz_cmd_lim, nz_neg_soft, nz_neg_hard);
	}

	const double q_outer_limit = cat.q_rate_limit * gs.limiter_gain;
	const double kp_alpha_outer = fbw_blend_value(2.4, 1.7, fbw_mode_blend) * gs.cmd_gain;
	const double ki_alpha_outer = fbw_blend_value(1.05, 0.60, fbw_mode_blend) * gs.hold_gain;
	double nz_limit_gain_scale = 1.0;
	if (fbw_stick_pitch_shaped > 0.0)
	{
		const double t = Common::limit((fbw_nz_f - nz_pos_soft) / fbw_max(nz_pos_limit - nz_pos_soft, 0.1), 0.0, 1.0);
		const double shaped_t = fbw_smoothstep01(t);
		nz_limit_gain_scale = fbw_blend_value(1.0, fbw_nz_limit_gain_floor, shaped_t);
	}
	const double kp_nz_outer = fbw_blend_value(0.34, 0.24, fbw_mode_blend) * gs.cmd_gain * nz_limit_gain_scale;
	const double ki_nz_outer = fbw_blend_value(0.11, 0.06, fbw_mode_blend) * gs.hold_gain * nz_limit_gain_scale;
	const double alpha_err_rad = Common::rad(fbw_alpha_cmd_lim_deg - fbw_alpha_f);
	const double nz_err = fbw_nz_cmd_lim - fbw_nz_f;
	const double q_ref_alpha_raw = kp_alpha_outer * alpha_err_rad + fbw_alpha_outer_int;
	const double q_ref_nz_raw = kp_nz_outer * nz_err + fbw_nz_outer_int;
	fbw_q_ref_alpha = Common::limit(q_ref_alpha_raw, -q_outer_limit, q_outer_limit);
	fbw_q_ref_nz = Common::limit(q_ref_nz_raw, -q_outer_limit, q_outer_limit);
	fbw_alpha_outer_int += (ki_alpha_outer * alpha_err_rad + fbw_outer_aw_gain * (fbw_q_ref_alpha - q_ref_alpha_raw)) * dt;
	fbw_nz_outer_int += (ki_nz_outer * nz_err + fbw_outer_aw_gain * (fbw_q_ref_nz - q_ref_nz_raw)) * dt;
	fbw_alpha_outer_int = Common::limit(fbw_alpha_outer_int, -fbw_outer_int_limit, fbw_outer_int_limit);
	fbw_nz_outer_int = Common::limit(fbw_nz_outer_int, -fbw_outer_int_limit, fbw_outer_int_limit);
	if (fbw_w_alpha < 0.05)
	{
		fbw_alpha_outer_int = fbw_first_order(fbw_alpha_outer_int, 0.0, 0.35, dt);
	}
	if (fbw_w_nz < 0.05)
	{
		fbw_nz_outer_int = fbw_first_order(fbw_nz_outer_int, 0.0, 0.35, dt);
	}

	fbw_q_cmd_direct = fbw_stick_pitch_shaped * Common::rad(fbw_q_cmd_land_max_deg) * gs.cmd_gain;
	fbw_q_ref_q = fbw_q_cmd_direct;
	fbw_q_ref_blended = fbw_w_alpha * fbw_q_ref_alpha + fbw_w_nz * fbw_q_ref_nz + fbw_w_q * fbw_q_ref_q;
	const double q_ref_prev = fbw_q_ref_filtered;
	fbw_q_ref_filtered = fbw_first_order(fbw_q_ref_filtered, fbw_q_ref_blended, fbw_pitch_ref_tau, dt);
	const double q_ref_step = Common::rad(fbw_pitch_ref_rate_deg_s) * dt;
	fbw_q_ref_filtered = Common::limit(fbw_q_ref_filtered, q_ref_prev - q_ref_step, q_ref_prev + q_ref_step);
	fbw_q_cmd_rate = Common::limit(fbw_q_ref_filtered, -q_outer_limit, q_outer_limit);
	fbw_aoa_limit_active = std::fabs(fbw_alpha_cmd_deg - fbw_alpha_cmd_lim_deg) > 0.05;
	fbw_g_limit_active = std::fabs(fbw_nz_cmd - fbw_nz_cmd_lim) > 0.02;

	const bool degrade_aoa = (alpha_abs > cat.alpha_hold_degrade_deg) || (alpha_abs > (alpha_limit_deg * 0.95));
	const bool degrade_qbar = fbw_qbar_f < cat.qbar_min_hold;
	const bool degrade_hold = hold_cmd_overlimit;

	if (fbw_state == FBW_STATE_HOLD && (degrade_aoa || degrade_qbar || degrade_hold))
	{
		fbw_state = FBW_STATE_DEGRADE;
		fbw_hold_active = false;
		fbw_hold_exit_reason = degrade_aoa ? FBW_HOLD_EXIT_AOA : (degrade_qbar ? FBW_HOLD_EXIT_QBAR : FBW_HOLD_EXIT_HOLD_CMD);
	}

	if (fbw_state == FBW_STATE_DEGRADE)
	{
		fbw_hold_gain_scale = fbw_first_order(fbw_hold_gain_scale, 0.0, cat.hold_decay_tau, dt);
		if (fbw_hold_gain_scale < 1e-3)
		{
			fbw_hold_gain_scale = 0.0;
		}
	}
	else
	{
		fbw_hold_gain_scale = 1.0;
	}

	const bool hold_path_active = stick_in_deadband && (fbw_state == FBW_STATE_HOLD || fbw_state == FBW_STATE_DEGRADE);
	if (hold_path_active)
	{
		fbw_p_cmd = fbw_p_cmd_hold * fbw_hold_gain_scale;
		fbw_q_cmd = fbw_q_cmd_hold * fbw_hold_gain_scale;
	}
	else
	{
		fbw_p_cmd = fbw_p_cmd_rate;
		fbw_q_cmd = fbw_q_cmd_rate;
	}

	const double beta_rad = fbw_beta_f / rad_to_deg;
	fbw_r_cmd_damper = -(cat.yaw_damper_beta * beta_rad + cat.yaw_damper_r * fbw_r_f) * gs.damping_gain;
	fbw_r_cmd = fbw_r_cmd_rate + fbw_r_cmd_damper;

	const double p_lim = cat.p_rate_limit * gs.limiter_gain;
	const double q_lim = cat.q_rate_limit * gs.limiter_gain;
	const double r_lim = cat.r_rate_limit * gs.limiter_gain;
	const double p_cmd_before_limit = fbw_p_cmd;
	const double q_cmd_before_limit = fbw_q_cmd;
	const double r_cmd_before_limit = fbw_r_cmd;
	fbw_p_cmd = Common::limit(fbw_p_cmd, -p_lim, p_lim);
	fbw_q_cmd = Common::limit(fbw_q_cmd, -q_lim, q_lim);
	fbw_r_cmd = Common::limit(fbw_r_cmd, -r_lim, r_lim);
	fbw_rate_limit_active =
		(std::fabs(p_cmd_before_limit - fbw_p_cmd) > 1e-5) ||
		(std::fabs(q_cmd_before_limit - fbw_q_cmd) > 1e-5) ||
		(std::fabs(r_cmd_before_limit - fbw_r_cmd) > 1e-5);

	fbw_p_err = fbw_p_cmd - fbw_p_f;
	fbw_q_err = fbw_q_cmd - fbw_q_f;
	fbw_r_err = fbw_r_cmd - fbw_r_f;

	const double kp_p = fbw_kp_p * gs.damping_gain;
	const double kp_q = fbw_kp_q * gs.damping_gain;
	const double kp_r = fbw_kp_r * gs.damping_gain;
	const double ki_p = fbw_ki_p * gs.damping_gain;
	const double ki_q = fbw_ki_q * gs.damping_gain;
	const double ki_r = fbw_ki_r * gs.damping_gain;

	const double ail_pre_norm = kp_p * fbw_p_err + ki_p * fbw_int_p;
	const double ele_pre_norm = kp_q * fbw_q_err + ki_q * fbw_int_q;
	const double rud_pre_norm = kp_r * fbw_r_err + ki_r * fbw_int_r;
	const double ail_sat_norm = Common::limit(ail_pre_norm, -1.0, 1.0);
	const double ele_sat_norm = Common::limit(ele_pre_norm, -1.0, 1.0);
	const double rud_sat_norm = Common::limit(rud_pre_norm, -1.0, 1.0);

	fbw_int_p += (fbw_p_err + fbw_aw_gain * (ail_sat_norm - ail_pre_norm)) * dt;
	fbw_int_q += (fbw_q_err + fbw_aw_gain * (ele_sat_norm - ele_pre_norm)) * dt;
	fbw_int_r += (fbw_r_err + fbw_aw_gain * (rud_sat_norm - rud_pre_norm)) * dt;
	fbw_int_p = Common::limit(fbw_int_p, -fbw_int_limit, fbw_int_limit);
	fbw_int_q = Common::limit(fbw_int_q, -fbw_int_limit, fbw_int_limit);
	fbw_int_r = Common::limit(fbw_int_r, -fbw_int_limit, fbw_int_limit);

	fbw_anti_windup_active =
		(std::fabs(ail_pre_norm - ail_sat_norm) > 1e-4) ||
		(std::fabs(ele_pre_norm - ele_sat_norm) > 1e-4) ||
		(std::fabs(rud_pre_norm - rud_sat_norm) > 1e-4);

	bool axis_sat_ail = false;
	bool axis_sat_ele = false;
	bool axis_sat_rud = false;
	aileron_command = fbw_apply_axis_actuator(
		ail_sat_norm, fbw_ail_limit_deg, fbw_ail_rate_deg_s, fbw_ail_lag_tau, dt,
		fbw_ail_rate_state_deg, fbw_ail_lag_state_deg,
		fbw_ail_cmd_pre, fbw_ail_cmd_sat, fbw_ail_cmd_rate, fbw_ail_cmd_lag, axis_sat_ail);
	elevator_command = fbw_apply_axis_actuator(
		ele_sat_norm, fbw_ele_limit_deg, fbw_ele_rate_deg_s, fbw_ele_lag_tau, dt,
		fbw_ele_rate_state_deg, fbw_ele_lag_state_deg,
		fbw_ele_cmd_pre, fbw_ele_cmd_sat, fbw_ele_cmd_rate, fbw_ele_cmd_lag, axis_sat_ele);
	rudder_command = fbw_apply_axis_actuator(
		rud_sat_norm, fbw_rud_limit_deg, fbw_rud_rate_deg_s, fbw_rud_lag_tau, dt,
		fbw_rud_rate_state_deg, fbw_rud_lag_state_deg,
		fbw_rud_cmd_pre, fbw_rud_cmd_sat, fbw_rud_cmd_rate, fbw_rud_cmd_lag, axis_sat_rud);

	fbw_actuator_sat = axis_sat_ail || axis_sat_ele || axis_sat_rud;
	if (fbw_actuator_sat)
	{
		fbw_actuator_sat_timer += dt;
	}
	else
	{
		fbw_actuator_sat_timer = Common::limit(fbw_actuator_sat_timer - dt, 0.0, 10.0);
	}

	if (fbw_state == FBW_STATE_HOLD && fbw_actuator_sat_timer > cat.sat_time)
	{
		fbw_state = FBW_STATE_DEGRADE;
		fbw_hold_active = false;
		fbw_hold_exit_reason = FBW_HOLD_EXIT_ACTUATOR_SAT;
	}

	return output;
}
}
