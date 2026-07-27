#include "ControlLawLifecycle.h"

namespace
{
void reset_hold_state(Systems::FBWControllerState& state, double roll, double pitch)
{
	state.mode_target = Systems::FBW_CAT1;
	state.mode_blend = 0.0;
	state.control_state = Systems::FBW_STATE_RATE;
	state.hold_active = false;
	state.hold_exit_reason = Systems::FBW_HOLD_EXIT_NONE;
	state.hold_enter_reason = 0;
	state.hold_timer = 0.0;
	state.hold_gain_scale = 1.0;
	state.phi_ref = roll;
	state.theta_ref = pitch;
}

void reset_rate_loop(Systems::FBWControllerState& state)
{
	state.int_p = 0.0;
	state.int_q = 0.0;
	state.int_r = 0.0;
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
}

void reset_pitch_loop(Systems::FBWControllerState& state, double alpha, double g)
{
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
}

void reset_actuators(Systems::FBWControllerState& state)
{
	state.ail_rate_state_deg = 0.0;
	state.ail_lag_state_deg = 0.0;
	state.ele_rate_state_deg = 0.0;
	state.ele_lag_state_deg = 0.0;
	state.rud_rate_state_deg = 0.0;
	state.rud_lag_state_deg = 0.0;
}

void reset_limiters(Systems::FBWControllerState& state)
{
	state.aoa_limit_active = false;
	state.rate_limit_active = false;
	state.actuator_sat = false;
	state.anti_windup_active = false;
	state.g_limit_active = false;
	state.actuator_sat_timer = 0.0;
}
}

namespace Systems
{
void reset_fbw_state(
	FBWControllerState& state,
	const FBWResetInput& input)
{
	reset_hold_state(state, input.roll, input.pitch);
	reset_rate_loop(state);
	reset_pitch_loop(state, input.alpha, input.g);
	reset_actuators(state);
	reset_limiters(state);
}

void reset_fbw_throttle_interface(FBWControllerState& state)
{
	state.throttle_cmd_left = 0.0;
	state.throttle_cmd_right = 0.0;
	state.throttle_blend = 0.0;
	state.throttle_override = false;
}
}
