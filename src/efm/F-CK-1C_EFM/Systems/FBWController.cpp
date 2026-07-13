#include "FBWController.h"

#include "FBWMath.h"
#include "../Common/Actuator.h"
#include "../Common/Clamp.h"
#include "../Common/Units.h"
#include <cmath>

namespace
{
constexpr double kDirectElevatorStep = 0.0125;
constexpr double kDirectAileronStep = 0.02;
constexpr double kDirectRudderStep = 0.012;
constexpr double kMetersPerSecondToKnots = 1.943844;
constexpr double kGearDownThreshold = 0.5;
constexpr double kApproachPitchRateWeight = 0.35;
constexpr double kPitchWeightEpsilon = 1e-6;
constexpr double kNegativeNzLimit = 2.5;
constexpr double kGLimiterOverrideMargin = 2.0;
constexpr double kNzPositiveBufferMinimum = 0.25;
constexpr double kNzNegativeSoftMinimum = 1.0;
constexpr double kNzNegativeSoftRatio = 0.65;
constexpr double kAlphaOuterKpCat1 = 2.4;
constexpr double kAlphaOuterKpCat3 = 1.7;
constexpr double kAlphaOuterKiCat1 = 1.05;
constexpr double kAlphaOuterKiCat3 = 0.60;
constexpr double kNzOuterKpCat1 = 0.34;
constexpr double kNzOuterKpCat3 = 0.24;
constexpr double kNzOuterKiCat1 = 0.11;
constexpr double kNzOuterKiCat3 = 0.06;
constexpr double kNzLimitRangeMinimum = 0.1;
constexpr double kInactivePitchWeight = 0.05;
constexpr double kInactiveIntegratorDecayTau = 0.35;
constexpr double kAoaLimitTolerance = 0.05;
constexpr double kGLimitTolerance = 0.02;
constexpr double kAoaDegradeRatio = 0.95;
constexpr double kHoldGainMinimum = 1e-3;
constexpr double kRateLimitTolerance = 1e-5;
constexpr double kAntiWindupTolerance = 1e-4;
constexpr double kActuatorTolerance = 1e-3;
constexpr double kActuatorTimerMaximum = 10.0;

struct FBWActuatorConfig
{
	double limit_deg;
	double rate_deg_s;
	double lag_tau;
};

struct FBWActuatorStateView
{
	double& rate_state_deg;
	double& lag_state_deg;
	double& debug_pre_deg;
	double& debug_sat_deg;
	double& debug_rate_deg;
	double& debug_lag_deg;
};

struct FBWActuatorResult
{
	double command;
	bool saturated;
};

struct FBWOuterPitchGains
{
	double kp_alpha;
	double ki_alpha;
	double kp_nz;
	double ki_nz;
};

class FBWFrame
{
public:
	FBWFrame(
		Systems::FBWControllerState& state,
		const Systems::FBWControllerConfig& config,
		const Systems::FBWControllerInput& input)
		: state_(state),
		config_(config),
		input_(input),
		output_{ input.elevator_command, input.aileron_command, input.rudder_command }
	{
	}

	Systems::FBWControllerOutput run()
	{
		if (!state_.enabled)
		{
			return update_direct_mode();
		}
		update_mode();
		capture_and_filter_signals();
		shape_stick_commands();
		update_hold_entry();
		prepare_rate_commands();
		update_pitch_weights();
		update_hold_commands();
		update_pitch_demands();
		update_outer_pitch_loop();
		update_pitch_reference();
		update_hold_degrade();
		select_rate_commands();
		limit_rate_commands();
		update_inner_rate_loop();
		update_actuators();
		return output_;
	}

private:
	Systems::FBWControllerOutput update_direct_mode()
	{
		output_.elevator_command = Common::limit(
			Common::actuator(output_.elevator_command, input_.pitch_input + input_.pitch_trim,
				-kDirectElevatorStep, kDirectElevatorStep),
			-1.0, 1.0);
		output_.aileron_command = Common::limit(
			Common::actuator(output_.aileron_command, input_.roll_input + input_.roll_trim,
				-kDirectAileronStep, kDirectAileronStep),
			-1.0, 1.0);
		output_.rudder_command = Common::limit(
			Common::actuator(output_.rudder_command, input_.yaw_input + input_.yaw_trim,
				-kDirectRudderStep, kDirectRudderStep),
			-1.0, 1.0);
		return output_;
	}

	void update_mode()
	{
		const double target = (state_.mode_target == Systems::FBW_CAT3) ? 1.0 : 0.0;
		state_.mode_blend = first_order(
			state_.mode_blend,
			target,
			config_.mode_switch_tau);
		cat_ = Systems::fbw_blend_cat_params(config_.cat1, config_.cat3, state_.mode_blend);
	}

	void capture_and_filter_signals()
	{
		state_.phi_raw = input_.roll;
		state_.theta_raw = input_.pitch;
		state_.p_raw = input_.roll_rate;
		state_.q_raw = input_.pitch_rate;
		state_.r_raw = input_.yaw_rate;
		state_.alpha_raw = input_.alpha;
		state_.beta_raw = input_.beta;
		state_.qbar_raw = input_.qbar;
		state_.ias_raw = input_.speed_scalar * kMetersPerSecondToKnots;
		state_.mach_raw = input_.mach;

		state_.phi_f = filter_signal(state_.phi_f, state_.phi_raw);
		state_.theta_f = filter_signal(state_.theta_f, state_.theta_raw);
		state_.p_f = filter_signal(state_.p_f, state_.p_raw);
		state_.q_f = filter_signal(state_.q_f, state_.q_raw);
		state_.r_f = filter_signal(state_.r_f, state_.r_raw);
		state_.alpha_f = filter_signal(state_.alpha_f, state_.alpha_raw);
		state_.beta_f = filter_signal(state_.beta_f, state_.beta_raw);
		state_.qbar_f = first_order(state_.qbar_f, state_.qbar_raw, config_.qbar_filter_tau);
		state_.ias_f = filter_signal(state_.ias_f, state_.ias_raw);
		state_.mach_f = filter_signal(state_.mach_f, state_.mach_raw);
	}

	double filter_signal(double current, double target) const
	{
		return first_order(current, target, config_.signal_filter_tau);
	}

	double first_order(double current, double target, double tau) const
	{
		if (tau <= 1e-6)
		{
			return target;
		}
		const double gain = Common::limit(input_.dt / (tau + input_.dt), 0.0, 1.0);
		return current + (target - current) * gain;
	}

	void shape_stick_commands()
	{
		state_.stick_roll_raw = Common::limit(input_.roll_input + input_.roll_trim, -1.0, 1.0);
		state_.stick_pitch_raw = Common::limit(input_.pitch_input + input_.pitch_trim, -1.0, 1.0);
		state_.stick_yaw_raw = Common::limit(input_.yaw_input + input_.yaw_trim, -1.0, 1.0);
		const double roll_target = shape_stick(state_.stick_roll_raw);
		const double pitch_target = shape_stick(state_.stick_pitch_raw);
		const double yaw_target = shape_stick(state_.stick_yaw_raw);
		const double roll_previous = state_.stick_roll_shaped;
		const double pitch_previous = state_.stick_pitch_shaped;
		const double yaw_previous = state_.stick_yaw_shaped;
		state_.stick_roll_shaped = filter_stick(state_.stick_roll_shaped, roll_target);
		state_.stick_pitch_shaped = filter_stick(state_.stick_pitch_shaped, pitch_target);
		state_.stick_yaw_shaped = filter_stick(state_.stick_yaw_shaped, yaw_target);
		const double maximum_step = cat_.command_shape_rate * input_.dt;
		state_.stick_roll_shaped = Common::limit(
			state_.stick_roll_shaped, roll_previous - maximum_step, roll_previous + maximum_step);
		state_.stick_pitch_shaped = Common::limit(
			state_.stick_pitch_shaped, pitch_previous - maximum_step, pitch_previous + maximum_step);
		state_.stick_yaw_shaped = Common::limit(
			state_.stick_yaw_shaped, yaw_previous - maximum_step, yaw_previous + maximum_step);
		stick_in_deadband_ = std::fabs(state_.stick_roll_raw) <= cat_.deadband &&
			std::fabs(state_.stick_pitch_raw) <= cat_.deadband;
	}

	double shape_stick(double value) const
	{
		return (1.0 - cat_.stick_expo) * value + cat_.stick_expo * value * value * value;
	}

	double filter_stick(double current, double target) const
	{
		return first_order(current, target, cat_.command_shape_tau);
	}

	void reset_hold_for_rate_mode()
	{
		state_.control_state = Systems::FBW_STATE_RATE;
		state_.hold_active = false;
		state_.hold_timer = 0.0;
		state_.hold_gain_scale = 1.0;
		state_.hold_exit_reason = Systems::FBW_HOLD_EXIT_STICK;
		state_.hold_enter_reason = 0;
	}

	void update_hold_entry()
	{
		if (input_.wow || !stick_in_deadband_)
		{
			reset_hold_for_rate_mode();
			return;
		}
		state_.hold_timer += input_.dt;
		if (state_.control_state != Systems::FBW_STATE_RATE ||
			state_.hold_timer < cat_.hold_engage_time)
		{
			return;
		}
		state_.control_state = Systems::FBW_STATE_HOLD;
		state_.hold_active = true;
		state_.phi_ref = state_.phi_f;
		state_.theta_ref = state_.theta_f;
		state_.hold_gain_scale = 1.0;
		state_.hold_exit_reason = Systems::FBW_HOLD_EXIT_NONE;
		state_.hold_enter_reason = 1;
	}

	void prepare_rate_commands()
	{
		gains_ = Systems::fbw_eval_gain_schedule(config_, state_.qbar_f);
		state_.nz_raw = input_.g;
		state_.nz_f = first_order(state_.nz_f, state_.nz_raw, config_.nz_filter_tau);
		state_.p_cmd_rate = state_.stick_roll_shaped * cat_.p_cmd_max * gains_.cmd_gain;
		state_.r_cmd_rate = state_.stick_yaw_shaped * cat_.r_cmd_max * gains_.cmd_gain;

		const bool pitch_stick_in_deadband = std::fabs(state_.stick_pitch_raw) <= cat_.deadband;
		if (input_.wow || pitch_stick_in_deadband)
		{
			state_.alpha_trim_deg = first_order(
				state_.alpha_trim_deg, state_.alpha_f, config_.alpha_trim_tau);
			state_.nz_trim_g = first_order(state_.nz_trim_g, state_.nz_f, config_.nz_trim_tau);
		}
	}

	void update_pitch_weights()
	{
		const double gear_weight = (input_.gear_pos > kGearDownThreshold) ? 1.0 : 0.0;
		const double approach = Common::limit(
			(config_.region_approach_kts - state_.ias_f) /
			(config_.region_approach_kts - config_.region_min_kts), 0.0, 1.0);
		high_speed_weight_ = Common::limit(
			(state_.ias_f - config_.region_low_kts) /
			(config_.region_high_kts - config_.region_low_kts), 0.0, 1.0);
		const double alpha_region = Common::limit(
			(std::fabs(state_.alpha_f) - config_.region_alpha1_deg) /
			(config_.region_alpha2_deg - config_.region_alpha1_deg), 0.0, 1.0);
		state_.w_q = gear_weight * Systems::fbw_blend_value(kApproachPitchRateWeight, 1.0, approach);
		state_.w_nz = high_speed_weight_ * (1.0 - state_.w_q);
		state_.w_alpha = Systems::fbw_max(1.0 - state_.w_nz - state_.w_q, alpha_region);
		const double sum = Systems::fbw_max(state_.w_alpha + state_.w_nz + state_.w_q, kPitchWeightEpsilon);
		state_.w_alpha /= sum;
		state_.w_nz /= sum;
		state_.w_q /= sum;
	}

	void update_hold_commands()
	{
		state_.phi_err = 0.0;
		state_.theta_err = 0.0;
		state_.p_cmd_hold = 0.0;
		state_.q_cmd_hold = 0.0;
		hold_cmd_overlimit_ = false;
		const bool hold_state = state_.control_state == Systems::FBW_STATE_HOLD ||
			state_.control_state == Systems::FBW_STATE_DEGRADE;
		if (!hold_state || !stick_in_deadband_)
		{
			return;
		}
		state_.phi_err = Systems::fbw_wrap_pi(state_.phi_ref - state_.phi_f);
		state_.theta_err = state_.theta_ref - state_.theta_f;
		const double p_raw = state_.phi_err * cat_.hold_phi_kp * gains_.hold_gain;
		const double q_raw = state_.theta_err * cat_.hold_theta_kp * gains_.hold_gain;
		const double p_limit = cat_.hold_p_cmd_max * gains_.limiter_gain;
		const double q_limit = cat_.hold_q_cmd_max * gains_.limiter_gain;
		state_.p_cmd_hold = Common::limit(p_raw, -p_limit, p_limit);
		state_.q_cmd_hold = Common::limit(q_raw, -q_limit, q_limit);
		hold_cmd_overlimit_ =
			std::fabs(p_raw) > p_limit * cat_.hold_cmd_ratio_limit ||
			std::fabs(q_raw) > q_limit * cat_.hold_cmd_ratio_limit;
	}

	void update_pitch_demands()
	{
		alpha_abs_ = std::fabs(state_.alpha_f);
		alpha_soft_ = Common::limit(cat_.aoa_soft_deg, 0.1, input_.alpha_limit_deg);
		const double alpha_range = Systems::fbw_blend_value(
			config_.alpha_cmd_per_stick_deg,
			config_.alpha_cmd_per_stick_deg * 0.65,
			high_speed_weight_);
		state_.alpha_cmd_deg = state_.alpha_trim_deg + state_.stick_pitch_shaped * alpha_range;
		state_.alpha_cmd_lim_deg = Systems::fbw_soft_limit_symmetric(state_.alpha_cmd_deg, alpha_soft_);

		nz_positive_limit_ = state_.g_limiter_override ?
			cat_.g_hard + kGLimiterOverrideMargin : cat_.g_hard;
		const double positive_buffer = Systems::fbw_max(
			kNzPositiveBufferMinimum, config_.nz_limit_buffer_bias);
		nz_positive_soft_ = Systems::fbw_min(cat_.g_soft, nz_positive_limit_ - positive_buffer);
		const double negative_hard = -kNegativeNzLimit;
		const double negative_soft = -Systems::fbw_max(
			kNzNegativeSoftMinimum, kNegativeNzLimit * kNzNegativeSoftRatio);
		state_.nz_cmd = state_.stick_pitch_shaped >= 0.0
			? 1.0 + state_.stick_pitch_shaped * (nz_positive_limit_ - 1.0)
			: 1.0 + state_.stick_pitch_shaped * (1.0 + kNegativeNzLimit);
		state_.nz_cmd_lim = state_.nz_cmd;
		if (state_.nz_cmd_lim > nz_positive_soft_)
		{
			state_.nz_cmd_lim = Systems::fbw_soft_clip_positive(
				state_.nz_cmd_lim, nz_positive_soft_, nz_positive_limit_);
		}
		if (state_.nz_cmd_lim < negative_soft)
		{
			state_.nz_cmd_lim = Systems::fbw_soft_clip_negative(
				state_.nz_cmd_lim, negative_soft, negative_hard);
		}
	}

	void update_outer_pitch_loop()
	{
		q_outer_limit_ = cat_.q_rate_limit * gains_.limiter_gain;
		const double kp_alpha = Systems::fbw_blend_value(
			kAlphaOuterKpCat1, kAlphaOuterKpCat3, state_.mode_blend) * gains_.cmd_gain;
		const double ki_alpha = Systems::fbw_blend_value(
			kAlphaOuterKiCat1, kAlphaOuterKiCat3, state_.mode_blend) * gains_.hold_gain;
		double nz_gain_scale = 1.0;
		if (state_.stick_pitch_shaped > 0.0)
		{
			const double ratio = Common::limit(
				(state_.nz_f - nz_positive_soft_) /
				Systems::fbw_max(nz_positive_limit_ - nz_positive_soft_, kNzLimitRangeMinimum), 0.0, 1.0);
			nz_gain_scale = Systems::fbw_blend_value(
				1.0, config_.nz_limit_gain_floor, Systems::fbw_smoothstep01(ratio));
		}
		const double kp_nz = Systems::fbw_blend_value(
			kNzOuterKpCat1, kNzOuterKpCat3, state_.mode_blend) * gains_.cmd_gain * nz_gain_scale;
		const double ki_nz = Systems::fbw_blend_value(
			kNzOuterKiCat1, kNzOuterKiCat3, state_.mode_blend) * gains_.hold_gain * nz_gain_scale;
		integrate_outer_pitch_loop({ kp_alpha, ki_alpha, kp_nz, ki_nz });
	}

	void integrate_outer_pitch_loop(const FBWOuterPitchGains& gains)
	{
		const double alpha_error = Common::rad(state_.alpha_cmd_lim_deg - state_.alpha_f);
		const double nz_error = state_.nz_cmd_lim - state_.nz_f;
		const double alpha_raw = gains.kp_alpha * alpha_error + state_.alpha_outer_int;
		const double nz_raw = gains.kp_nz * nz_error + state_.nz_outer_int;
		state_.q_ref_alpha = Common::limit(alpha_raw, -q_outer_limit_, q_outer_limit_);
		state_.q_ref_nz = Common::limit(nz_raw, -q_outer_limit_, q_outer_limit_);
		state_.alpha_outer_int += (gains.ki_alpha * alpha_error +
			config_.outer_aw_gain * (state_.q_ref_alpha - alpha_raw)) * input_.dt;
		state_.nz_outer_int += (gains.ki_nz * nz_error +
			config_.outer_aw_gain * (state_.q_ref_nz - nz_raw)) * input_.dt;
		state_.alpha_outer_int = Common::limit(
			state_.alpha_outer_int, -config_.outer_int_limit, config_.outer_int_limit);
		state_.nz_outer_int = Common::limit(
			state_.nz_outer_int, -config_.outer_int_limit, config_.outer_int_limit);
		if (state_.w_alpha < kInactivePitchWeight)
		{
			state_.alpha_outer_int = first_order(
				state_.alpha_outer_int, 0.0, kInactiveIntegratorDecayTau);
		}
		if (state_.w_nz < kInactivePitchWeight)
		{
			state_.nz_outer_int = first_order(
				state_.nz_outer_int, 0.0, kInactiveIntegratorDecayTau);
		}
	}

	void update_pitch_reference()
	{
		state_.q_cmd_direct = state_.stick_pitch_shaped *
			Common::rad(config_.q_cmd_land_max_deg) * gains_.cmd_gain;
		state_.q_ref_q = state_.q_cmd_direct;
		state_.q_ref_blended = state_.w_alpha * state_.q_ref_alpha +
			state_.w_nz * state_.q_ref_nz + state_.w_q * state_.q_ref_q;
		const double previous = state_.q_ref_filtered;
		state_.q_ref_filtered = first_order(
			state_.q_ref_filtered, state_.q_ref_blended, config_.pitch_ref_tau);
		const double maximum_step = Common::rad(config_.pitch_ref_rate_deg_s) * input_.dt;
		state_.q_ref_filtered = Common::limit(
			state_.q_ref_filtered, previous - maximum_step, previous + maximum_step);
		state_.q_cmd_rate = Common::limit(state_.q_ref_filtered, -q_outer_limit_, q_outer_limit_);
		state_.aoa_limit_active =
			std::fabs(state_.alpha_cmd_deg - state_.alpha_cmd_lim_deg) > kAoaLimitTolerance;
		state_.g_limit_active = std::fabs(state_.nz_cmd - state_.nz_cmd_lim) > kGLimitTolerance;
	}

	void update_hold_degrade()
	{
		const bool aoa = alpha_abs_ > cat_.alpha_hold_degrade_deg ||
			alpha_abs_ > input_.alpha_limit_deg * kAoaDegradeRatio;
		const bool qbar = state_.qbar_f < cat_.qbar_min_hold;
		if (state_.control_state == Systems::FBW_STATE_HOLD && (aoa || qbar || hold_cmd_overlimit_))
		{
			state_.control_state = Systems::FBW_STATE_DEGRADE;
			state_.hold_active = false;
			state_.hold_exit_reason = aoa ? Systems::FBW_HOLD_EXIT_AOA :
				(qbar ? Systems::FBW_HOLD_EXIT_QBAR : Systems::FBW_HOLD_EXIT_HOLD_CMD);
		}
		if (state_.control_state != Systems::FBW_STATE_DEGRADE)
		{
			state_.hold_gain_scale = 1.0;
			return;
		}
		state_.hold_gain_scale = first_order(state_.hold_gain_scale, 0.0, cat_.hold_decay_tau);
		if (state_.hold_gain_scale < kHoldGainMinimum)
		{
			state_.hold_gain_scale = 0.0;
		}
	}

	void select_rate_commands()
	{
		const bool hold_path = stick_in_deadband_ &&
			(state_.control_state == Systems::FBW_STATE_HOLD ||
			state_.control_state == Systems::FBW_STATE_DEGRADE);
		state_.p_cmd = hold_path ?
			state_.p_cmd_hold * state_.hold_gain_scale : state_.p_cmd_rate;
		state_.q_cmd = hold_path ?
			state_.q_cmd_hold * state_.hold_gain_scale : state_.q_cmd_rate;
		const double beta_radians = state_.beta_f / Common::kDegPerRad;
		state_.r_cmd_damper = -(
			cat_.yaw_damper_beta * beta_radians + cat_.yaw_damper_r * state_.r_f) *
			gains_.damping_gain;
		state_.r_cmd = state_.r_cmd_rate + state_.r_cmd_damper;
	}

	void limit_rate_commands()
	{
		const double p_limit = cat_.p_rate_limit * gains_.limiter_gain;
		const double q_limit = cat_.q_rate_limit * gains_.limiter_gain;
		const double r_limit = cat_.r_rate_limit * gains_.limiter_gain;
		const double p_before = state_.p_cmd;
		const double q_before = state_.q_cmd;
		const double r_before = state_.r_cmd;
		state_.p_cmd = Common::limit(state_.p_cmd, -p_limit, p_limit);
		state_.q_cmd = Common::limit(state_.q_cmd, -q_limit, q_limit);
		state_.r_cmd = Common::limit(state_.r_cmd, -r_limit, r_limit);
		state_.rate_limit_active =
			std::fabs(p_before - state_.p_cmd) > kRateLimitTolerance ||
			std::fabs(q_before - state_.q_cmd) > kRateLimitTolerance ||
			std::fabs(r_before - state_.r_cmd) > kRateLimitTolerance;
		state_.p_err = state_.p_cmd - state_.p_f;
		state_.q_err = state_.q_cmd - state_.q_f;
		state_.r_err = state_.r_cmd - state_.r_f;
	}

	void update_inner_rate_loop()
	{
		const double kp_p = config_.kp_p * gains_.damping_gain;
		const double kp_q = config_.kp_q * gains_.damping_gain;
		const double kp_r = config_.kp_r * gains_.damping_gain;
		const double ki_p = config_.ki_p * gains_.damping_gain;
		const double ki_q = config_.ki_q * gains_.damping_gain;
		const double ki_r = config_.ki_r * gains_.damping_gain;
		const double aileron_pre = kp_p * state_.p_err + ki_p * state_.int_p;
		const double elevator_pre = kp_q * state_.q_err + ki_q * state_.int_q;
		const double rudder_pre = kp_r * state_.r_err + ki_r * state_.int_r;
		aileron_command_ = Common::limit(aileron_pre, -1.0, 1.0);
		elevator_command_ = Common::limit(elevator_pre, -1.0, 1.0);
		rudder_command_ = Common::limit(rudder_pre, -1.0, 1.0);
		state_.int_p += (state_.p_err + config_.aw_gain * (aileron_command_ - aileron_pre)) * input_.dt;
		state_.int_q += (state_.q_err + config_.aw_gain * (elevator_command_ - elevator_pre)) * input_.dt;
		state_.int_r += (state_.r_err + config_.aw_gain * (rudder_command_ - rudder_pre)) * input_.dt;
		state_.int_p = Common::limit(state_.int_p, -config_.int_limit, config_.int_limit);
		state_.int_q = Common::limit(state_.int_q, -config_.int_limit, config_.int_limit);
		state_.int_r = Common::limit(state_.int_r, -config_.int_limit, config_.int_limit);
		state_.anti_windup_active =
			std::fabs(aileron_pre - aileron_command_) > kAntiWindupTolerance ||
			std::fabs(elevator_pre - elevator_command_) > kAntiWindupTolerance ||
			std::fabs(rudder_pre - rudder_command_) > kAntiWindupTolerance;
	}

	FBWActuatorResult apply_axis_actuator(
		double normalized_command,
		const FBWActuatorConfig& actuator,
		FBWActuatorStateView view)
	{
		view.debug_pre_deg = normalized_command * actuator.limit_deg;
		view.debug_sat_deg = Common::limit(
			view.debug_pre_deg, -actuator.limit_deg, actuator.limit_deg);
		const double maximum_step = actuator.rate_deg_s * input_.dt;
		view.rate_state_deg += Common::limit(
			view.debug_sat_deg - view.rate_state_deg, -maximum_step, maximum_step);
		view.rate_state_deg = Common::limit(
			view.rate_state_deg, -actuator.limit_deg, actuator.limit_deg);
		const double lag_gain = Common::limit(
			input_.dt / (actuator.lag_tau + input_.dt), 0.0, 1.0);
		view.lag_state_deg += (view.rate_state_deg - view.lag_state_deg) * lag_gain;
		view.lag_state_deg = Common::limit(
			view.lag_state_deg, -actuator.limit_deg, actuator.limit_deg);
		view.debug_rate_deg = view.rate_state_deg;
		view.debug_lag_deg = view.lag_state_deg;
		const bool saturated =
			std::fabs(view.debug_pre_deg - view.debug_sat_deg) > kActuatorTolerance ||
			std::fabs(view.debug_sat_deg - view.rate_state_deg) > kActuatorTolerance ||
			std::fabs(view.lag_state_deg) > actuator.limit_deg - kActuatorTolerance;
		return {
			Common::limit(view.lag_state_deg / actuator.limit_deg, -1.0, 1.0),
			saturated
		};
	}

	void update_actuators()
	{
		const FBWActuatorResult aileron = apply_axis_actuator(
			aileron_command_,
			{ config_.ail_limit_deg, config_.ail_rate_deg_s, config_.ail_lag_tau },
			{ state_.ail_rate_state_deg, state_.ail_lag_state_deg,
				state_.ail_cmd_pre, state_.ail_cmd_sat, state_.ail_cmd_rate, state_.ail_cmd_lag });
		const FBWActuatorResult elevator = apply_axis_actuator(
			elevator_command_,
			{ config_.ele_limit_deg, config_.ele_rate_deg_s, config_.ele_lag_tau },
			{ state_.ele_rate_state_deg, state_.ele_lag_state_deg,
				state_.ele_cmd_pre, state_.ele_cmd_sat, state_.ele_cmd_rate, state_.ele_cmd_lag });
		const FBWActuatorResult rudder = apply_axis_actuator(
			rudder_command_,
			{ config_.rud_limit_deg, config_.rud_rate_deg_s, config_.rud_lag_tau },
			{ state_.rud_rate_state_deg, state_.rud_lag_state_deg,
				state_.rud_cmd_pre, state_.rud_cmd_sat, state_.rud_cmd_rate, state_.rud_cmd_lag });
		output_.aileron_command = aileron.command;
		output_.elevator_command = elevator.command;
		output_.rudder_command = rudder.command;
		state_.actuator_sat = aileron.saturated || elevator.saturated || rudder.saturated;
		state_.actuator_sat_timer = state_.actuator_sat
			? state_.actuator_sat_timer + input_.dt
			: Common::limit(state_.actuator_sat_timer - input_.dt, 0.0, kActuatorTimerMaximum);
		if (state_.control_state == Systems::FBW_STATE_HOLD &&
			state_.actuator_sat_timer > cat_.sat_time)
		{
			state_.control_state = Systems::FBW_STATE_DEGRADE;
			state_.hold_active = false;
			state_.hold_exit_reason = Systems::FBW_HOLD_EXIT_ACTUATOR_SAT;
		}
	}

	Systems::FBWControllerState& state_;
	const Systems::FBWControllerConfig& config_;
	const Systems::FBWControllerInput& input_;
	Systems::FBWControllerOutput output_;
	Systems::FBWCatParams cat_;
	Systems::FBWGainScheduleValues gains_;
	bool stick_in_deadband_ = false;
	bool hold_cmd_overlimit_ = false;
	double high_speed_weight_ = 0.0;
	double alpha_abs_ = 0.0;
	double alpha_soft_ = 0.0;
	double nz_positive_limit_ = 0.0;
	double nz_positive_soft_ = 0.0;
	double q_outer_limit_ = 0.0;
	double aileron_command_ = 0.0;
	double elevator_command_ = 0.0;
	double rudder_command_ = 0.0;
};
}

namespace Systems
{
void set_fbw_cat_mode(FBWControllerState& state, FBWCatMode mode)
{
	state.mode_target = mode;
}

void toggle_fbw_cat_mode(FBWControllerState& state, bool command_pressed)
{
	if (command_pressed)
	{
		state.mode_target = (state.mode_target == FBW_CAT1) ? FBW_CAT3 : FBW_CAT1;
	}
}

void set_fbw_g_limiter_override(FBWControllerState& state, bool enabled)
{
	state.g_limiter_override = enabled;
}

void toggle_fbw_g_limiter_override(FBWControllerState& state, bool command_pressed)
{
	if (command_pressed)
	{
		state.g_limiter_override = !state.g_limiter_override;
	}
}

const char* fbw_mode_name(const FBWControllerState& state)
{
	return (state.mode_blend >= 0.5) ? "CAT3" : "CAT1";
}

const char* fbw_state_name(const FBWControllerState& state)
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

const char* fbw_exit_reason_name(const FBWControllerState& state)
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

FBWControllerOutput update_fbw_controller(
	FBWControllerState& state,
	const FBWControllerConfig& config,
	const FBWControllerInput& input)
{
	return FBWFrame(state, config, input).run();
}
}
