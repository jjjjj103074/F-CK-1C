#include "ControlLaws.h"

#include "ControlLawMath.h"
#include "ConfigurationAndMode.h"
#include "InnerLoopControl.h"
#include "Common/Actuator.h"
#include "Common/Clamp.h"
#include "Common/Units.h"
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
constexpr double kActuatorTimerMaximum = 10.0;

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
		const Systems::ConditionedFlightControlInput& input)
		: state_(state),
		config_(config),
		input_(input),
		output_{ input.elevator_position_normalized,
			input.aileron_position_normalized,
			input.rudder_position_normalized }
	{
	}

	Systems::FlightControlLawResult run()
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
		update_actuator_feedback();
		return output_;
	}

private:
	Systems::FlightControlLawResult update_direct_mode()
	{
		output_.surface_demand.elevator_command_normalized = Common::limit(
			Common::actuator(
				output_.surface_demand.elevator_command_normalized,
				{ input_.pilot_pitch_raw_normalized,
					-kDirectElevatorStep, kDirectElevatorStep }),
			-1.0, 1.0);
		output_.surface_demand.aileron_command_normalized = Common::limit(
			Common::actuator(
				output_.surface_demand.aileron_command_normalized,
				{ input_.pilot_roll_raw_normalized,
					-kDirectAileronStep, kDirectAileronStep }),
			-1.0, 1.0);
		output_.surface_demand.rudder_command_normalized = Common::limit(
			Common::actuator(
				output_.surface_demand.rudder_command_normalized,
				{ input_.pilot_yaw_raw_normalized,
					-kDirectRudderStep, kDirectRudderStep }),
			-1.0, 1.0);
		return output_;
	}

	void update_mode()
	{
		state_.mode_blend = input_.cat_mode_blend;
		cat_ = Systems::fbw_blend_cat_params(config_.cat1, config_.cat3, state_.mode_blend);
		envelope_ = Systems::make_maneuver_envelope(
			config_,
			{ cat_, input_.alpha_limit_deg, state_.g_limiter_override });
	}

	void capture_and_filter_signals()
	{
		state_.phi_raw = input_.roll_attitude_rad;
		state_.theta_raw = input_.pitch_attitude_rad;
		state_.p_raw = input_.roll_rate_rad_s;
		state_.q_raw = input_.pitch_rate_rad_s;
		state_.r_raw = input_.yaw_rate_rad_s;
		state_.alpha_raw = input_.angle_of_attack_deg;
		state_.beta_raw = input_.sideslip_deg;
		state_.qbar_raw = input_.dynamic_pressure_pa;
		state_.ias_raw = input_.indicated_airspeed_mps * kMetersPerSecondToKnots;
		state_.mach_raw = input_.mach;

		state_.phi_f = state_.phi_raw;
		state_.theta_f = state_.theta_raw;
		state_.p_f = state_.p_raw;
		state_.q_f = state_.q_raw;
		state_.r_f = state_.r_raw;
		state_.alpha_f = state_.alpha_raw;
		state_.beta_f = state_.beta_raw;
		state_.qbar_f = state_.qbar_raw;
		state_.ias_f = state_.ias_raw;
		state_.mach_f = state_.mach_raw;
	}

	double first_order(double current, double target, double tau) const
	{
		if (tau <= 1e-6)
		{
			return target;
		}
		const double gain = Common::limit(
			input_.dt_s / (tau + input_.dt_s), 0.0, 1.0);
		return current + (target - current) * gain;
	}

	void shape_stick_commands()
	{
		state_.stick_roll_raw = input_.pilot_roll_raw_normalized;
		state_.stick_pitch_raw = input_.pilot_pitch_raw_normalized;
		state_.stick_yaw_raw = input_.pilot_yaw_raw_normalized;
		state_.stick_roll_shaped = input_.pilot_roll_normalized;
		state_.stick_pitch_shaped = input_.pilot_pitch_normalized;
		state_.stick_yaw_shaped = input_.pilot_yaw_normalized;
		stick_in_deadband_ = input_.roll_pitch_in_deadband;
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
		if (input_.weight_on_wheels || !stick_in_deadband_)
		{
			reset_hold_for_rate_mode();
			return;
		}
		state_.hold_timer += input_.dt_s;
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
		state_.nz_raw = input_.normal_acceleration_g;
		state_.nz_f = state_.nz_raw;
		state_.p_cmd_rate = state_.stick_roll_shaped * cat_.p_cmd_max * gains_.cmd_gain;
		state_.r_cmd_rate = state_.stick_yaw_shaped * cat_.r_cmd_max * gains_.cmd_gain;

		if (input_.weight_on_wheels || input_.pitch_in_deadband)
		{
			state_.alpha_trim_deg = first_order(
				state_.alpha_trim_deg, state_.alpha_f, config_.alpha_trim_tau);
			state_.nz_trim_g = first_order(state_.nz_trim_g, state_.nz_f, config_.nz_trim_tau);
		}
	}

	void update_pitch_weights()
	{
		const double gear_weight =
			(input_.gear_position_normalized > kGearDownThreshold) ? 1.0 : 0.0;
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
		alpha_soft_ = Common::limit(
			cat_.aoa_soft_deg,
			0.1,
			envelope_.hard_protection.angle_of_attack_limit_deg);
		const double alpha_range = Systems::fbw_blend_value(
			config_.alpha_cmd_per_stick_deg,
			config_.alpha_cmd_per_stick_deg * 0.65,
			high_speed_weight_);
		state_.alpha_cmd_deg = state_.alpha_trim_deg + state_.stick_pitch_shaped * alpha_range;
		state_.alpha_cmd_lim_deg = Systems::fbw_soft_limit_symmetric(state_.alpha_cmd_deg, alpha_soft_);

		nz_positive_limit_ =
			envelope_.hard_protection.maximum_normal_acceleration_g;
		const double positive_buffer = Systems::fbw_max(
			kNzPositiveBufferMinimum, config_.nz_limit_buffer_bias);
		nz_positive_soft_ = Systems::fbw_min(cat_.g_soft, nz_positive_limit_ - positive_buffer);
		const double negative_hard =
			envelope_.hard_protection.minimum_normal_acceleration_g;
		const double negative_soft = -Systems::fbw_max(
			kNzNegativeSoftMinimum,
			-1.0 * negative_hard * kNzNegativeSoftRatio);
		state_.nz_cmd = state_.stick_pitch_shaped >= 0.0
			? 1.0 + state_.stick_pitch_shaped * (nz_positive_limit_ - 1.0)
			: 1.0 + state_.stick_pitch_shaped * (1.0 - negative_hard);
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
			config_.outer_aw_gain * (state_.q_ref_alpha - alpha_raw)) * input_.dt_s;
		state_.nz_outer_int += (gains.ki_nz * nz_error +
			config_.outer_aw_gain * (state_.q_ref_nz - nz_raw)) * input_.dt_s;
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
		const double maximum_step =
			Common::rad(config_.pitch_ref_rate_deg_s) * input_.dt_s;
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
			state_.hold_exit_reason = hold_degrade_reason(aoa, qbar);
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

	static Systems::FBWHoldExitReason hold_degrade_reason(bool aoa, bool qbar)
	{
		if (aoa)
		{
			return Systems::FBW_HOLD_EXIT_AOA;
		}
		return qbar
			? Systems::FBW_HOLD_EXIT_QBAR
			: Systems::FBW_HOLD_EXIT_HOLD_CMD;
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
		const Systems::LimitedBodyRateReference limited =
			Systems::limit_body_rate_reference(
				{ state_.p_cmd, state_.q_cmd, state_.r_cmd },
				{ envelope_.hard_protection.roll_rate_limit_rad_s *
					gains_.limiter_gain,
					envelope_.hard_protection.pitch_rate_limit_rad_s *
					gains_.limiter_gain,
					envelope_.hard_protection.yaw_rate_limit_rad_s *
					gains_.limiter_gain });
		state_.p_cmd = limited.value.roll_rate_rad_s;
		state_.q_cmd = limited.value.pitch_rate_rad_s;
		state_.r_cmd = limited.value.yaw_rate_rad_s;
		state_.rate_limit_active = limited.constrained;
		state_.p_err = state_.p_cmd - state_.p_f;
		state_.q_err = state_.q_cmd - state_.q_f;
		state_.r_err = state_.r_cmd - state_.r_f;
	}

	void update_inner_rate_loop()
	{
		const Systems::InnerRateLoopResult result =
			Systems::update_inner_rate_loop(
				{ state_.int_p, state_.int_q, state_.int_r },
				{ input_.dt_s,
					{ state_.p_cmd, state_.q_cmd, state_.r_cmd },
					{ state_.p_f, state_.q_f, state_.r_f },
					{ config_.kp_p * gains_.damping_gain,
						config_.ki_p * gains_.damping_gain,
						config_.kp_q * gains_.damping_gain,
						config_.ki_q * gains_.damping_gain,
						config_.kp_r * gains_.damping_gain,
						config_.ki_r * gains_.damping_gain,
						config_.aw_gain,
						config_.int_limit } });
		state_.int_p = result.state.roll_integral;
		state_.int_q = result.state.pitch_integral;
		state_.int_r = result.state.yaw_integral;
		state_.anti_windup_active = result.anti_windup_active;
		output_.surface_demand = result.surface_demand;
	}

	void update_actuator_feedback()
	{
		state_.actuator_sat = input_.actuator_saturated;
		state_.actuator_sat_timer = state_.actuator_sat
			? state_.actuator_sat_timer + input_.dt_s
			: Common::limit(
				state_.actuator_sat_timer - input_.dt_s,
				0.0,
				kActuatorTimerMaximum);
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
	const Systems::ConditionedFlightControlInput& input_;
	Systems::FlightControlLawResult output_;
	Systems::FBWCatParams cat_;
	Systems::ManeuverEnvelope envelope_;
	Systems::FBWGainScheduleValues gains_;
	bool stick_in_deadband_ = false;
	bool hold_cmd_overlimit_ = false;
	double high_speed_weight_ = 0.0;
	double alpha_abs_ = 0.0;
	double alpha_soft_ = 0.0;
	double nz_positive_limit_ = 0.0;
	double nz_positive_soft_ = 0.0;
	double q_outer_limit_ = 0.0;
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

FlightControlLawResult update_fbw_controller(
	FBWControllerState& state,
	const FBWControllerConfig& config,
	const ConditionedFlightControlInput& input)
{
	return FBWFrame(state, config, input).run();
}
}
