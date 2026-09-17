#include "LongitudinalControlLaw.h"

#include "ControlLawMath.h"
#include "LongitudinalCommandProtection.h"
#include "Common/Clamp.h"

#include <cmath>
#include <stdexcept>

namespace
{
constexpr double kMinimumTimeConstantS = 1.0e-6;
constexpr double kCommandDifferenceTolerance = 1.0e-9;
constexpr double kEffortDifferenceTolerance = 1.0e-5;

struct LoadFactorLimitResult
{
	double target_g = 1.0;
	bool limited = false;
};

struct FeedbackEffort
{
	double normal_acceleration = 0.0;
	double pitch_rate = 0.0;
	double angle_of_attack = 0.0;
};

struct FirstOrderInput
{
	double current = 0.0;
	double target = 0.0;
	double time_constant_s = 0.0;
	double dt_s = 0.0;
};

struct TrackingInput
{
	double raw_effort = 0.0;
	double limited_effort = 0.0;
	double protected_effort = 0.0;
	bool protection_active = false;
};

struct NormalAccelerationResultInput
{
	double requested_g = 1.0;
	LoadFactorLimitResult load;
	Systems::NormalAccelerationProtectionResult alpha;
	double error_g = 0.0;
	FeedbackEffort feedback;
};

double first_order(const FirstOrderInput& input)
{
	if (input.time_constant_s <= kMinimumTimeConstantS) return input.target;
	const double gain = Common::limit(
		input.dt_s / (input.time_constant_s + input.dt_s), 0.0, 1.0);
	return input.current + (input.target - input.current) * gain;
}

class LongitudinalFrame
{
public:
	LongitudinalFrame(
		Systems::LongitudinalControlLawState& state,
		const Systems::FlightControlLawsConfig& config,
		const Systems::FlightControlLawsInput& input)
		: state_(state),
		  config_(config.longitudinal),
		  stabilator_limit_rad_(
			  config.surface_mixer.symmetric_stabilator_limit_rad),
		  input_(input) {}

	Systems::LongitudinalAxisResult run()
	{
		if (std::holds_alternative<Core::Systems::NormalAccelerationCommand>(
			input_.maneuver.longitudinal.command))
		{
			return update_normal_acceleration();
		}
		return update_pitch_rate();
	}

private:
	LoadFactorLimitResult limit_load_factor(double requested_g) const
	{
		const auto& envelope = input_.configuration.envelope.hard_protection;
		const double buffer_g = Systems::fbw_max(
			config_.positive_buffer_minimum_g, config_.limit_buffer_bias_g);
		const double soft_positive_g = Systems::fbw_min(
			input_.configuration.stores.envelope.soft_positive_load_factor_g,
			envelope.maximum_normal_acceleration_g - buffer_g);
		const double soft_negative_g = -Systems::fbw_max(
			config_.negative_soft_minimum_g,
			-envelope.minimum_normal_acceleration_g *
				config_.negative_soft_ratio);
		double limited_g = requested_g;
		if (requested_g > soft_positive_g)
		{
			limited_g = Systems::fbw_soft_clip_positive(
				requested_g, soft_positive_g,
				envelope.maximum_normal_acceleration_g);
		}
		else if (requested_g < soft_negative_g)
		{
			limited_g = Systems::fbw_soft_clip_negative(
				requested_g, soft_negative_g,
				envelope.minimum_normal_acceleration_g);
		}
		return { limited_g,
			std::fabs(limited_g - requested_g) >
				kCommandDifferenceTolerance };
	}

	Systems::AlphaProtectionSchedule alpha_schedule() const
	{
		const auto& envelope = input_.configuration.envelope.hard_protection;
		return { input_.flight.angle_of_attack_rad,
			envelope.angle_of_attack_blend_start_rad,
			envelope.angle_of_attack_limit_rad };
	}

	double alpha_feedback_effort(double schedule_gain = 1.0) const
	{
		return -config_.angle_of_attack_stability_gain_rad_inv *
			input_.flight.angle_of_attack_rad *
			input_.configuration.gains.damping_gain * schedule_gain;
	}

	bool prepare_mode(
		Core::Systems::LongitudinalCommandMode mode,
		double non_integral_effort)
	{
		if (!state_.initialized)
		{
			state_.initialized = true;
			state_.active_mode = mode;
			state_.pitch_rate_low_pass_rad_s = input_.flight.pitch_rate_rad_s;
			return false;
		}
		const bool changed = state_.active_mode != mode;
		if (!changed) return false;
		state_.initialized = true;
		state_.active_mode = mode;
		state_.pitch_rate_low_pass_rad_s = input_.flight.pitch_rate_rad_s;
		const double tracked = actual_effort();
		if (mode == Core::Systems::LongitudinalCommandMode::NormalAcceleration)
		{
			state_.normal_acceleration_integral_effort = Common::limit(
				tracked - non_integral_effort,
				-config_.normal_acceleration_integral_limit_effort,
				config_.normal_acceleration_integral_limit_effort);
		}
		else
		{
			state_.pitch_rate_integral_effort = Common::limit(
				tracked - non_integral_effort,
				-config_.pitch_rate_command_integral_limit_effort,
				config_.pitch_rate_command_integral_limit_effort);
		}
		return changed;
	}

	double actual_effort() const
	{
		// This is the inverse electronic mixer mapping used for bumpless
		// tracking. Physical travel-limit ownership remains in the actuator.
		return Common::limit(
			input_.signals.symmetric_stabilator_position_rad /
				stabilator_limit_rad_,
			-1.0,
			1.0);
	}

	bool drives_physical_stop(double raw_effort) const
	{
		if (!input_.signals.symmetric_stabilator_at_position_limit) return false;
		const double actual = actual_effort();
		switch (input_.signals.symmetric_stabilator_position_limit)
		{
		case Core::FlightControlPositionLimit::None:
			return false;
		case Core::FlightControlPositionLimit::Negative:
			return raw_effort < actual;
		case Core::FlightControlPositionLimit::Positive:
			return raw_effort > actual;
		}
		throw std::logic_error("Unknown stabilator position-limit state.");
	}

	double tracking_correction(const TrackingInput& input) const
	{
		// A normal servo-rate lag is plant dynamics, not lost control authority.
		// Command protection owns the active electronic objective, so it sheds
		// stored pilot-command integral by tracking the protected non-integral
		// effort. Normal servo lag is plant dynamics; only a confirmed physical
		// stop tracks actual surface position.
		const bool physical_stop = drives_physical_stop(input.raw_effort);
		const double target = physical_stop ? actual_effort()
			: input.protection_active ? input.protected_effort
			: input.limited_effort;
		return target - input.raw_effort;
	}

	void update_normal_integral(
		double error_g,
		const TrackingInput& tracking)
	{
		const double blend = input_.configuration.stores_transition_0_1;
		const double ki = Systems::fbw_blend_value(
			config_.normal_acceleration_integral_cat1_s_inv,
			config_.normal_acceleration_integral_cat3_s_inv, blend);
		const double correction = tracking_correction(tracking);
		state_.normal_acceleration_integral_effort +=
			(ki * error_g + config_.normal_acceleration_anti_windup_s_inv *
				correction) * input_.flight.dt_s;
		state_.normal_acceleration_integral_effort = Common::limit(
			state_.normal_acceleration_integral_effort,
			-config_.normal_acceleration_integral_limit_effort,
			config_.normal_acceleration_integral_limit_effort);
	}

	void update_pitch_integral(
		double error_rad_s,
		const TrackingInput& tracking)
	{
		const double correction = tracking_correction(tracking);
		state_.pitch_rate_integral_effort +=
			(config_.pitch_rate_command_integral_gain_rad_inv * error_rad_s +
				config_.pitch_rate_command_anti_windup_s_inv * correction) *
			input_.flight.dt_s;
		state_.pitch_rate_integral_effort = Common::limit(
			state_.pitch_rate_integral_effort,
			-config_.pitch_rate_command_integral_limit_effort,
			config_.pitch_rate_command_integral_limit_effort);
	}

	Systems::LongitudinalAxisResult update_normal_acceleration()
	{
		const double requested_g = std::get<
			Core::Systems::NormalAccelerationCommand>(
				input_.maneuver.longitudinal.command).target_g;
		const LoadFactorLimitResult load = limit_load_factor(requested_g);
		const auto alpha = Systems::protect_normal_acceleration_command({
			load.target_g,
			input_.configuration.envelope.hard_protection.
				maximum_normal_acceleration_g,
			alpha_schedule(), config_ });
		return synthesize_normal_acceleration(requested_g, load, alpha);
	}

	Systems::LongitudinalAxisResult synthesize_normal_acceleration(
		double requested_g,
		const LoadFactorLimitResult& load,
		const Systems::NormalAccelerationProtectionResult& alpha)
	{
		const double error_g =
			alpha.protected_g - input_.flight.normal_acceleration_g;
		const bool entering_mode = !state_.initialized ||
			state_.active_mode !=
				Core::Systems::LongitudinalCommandMode::NormalAcceleration;
		state_.pitch_rate_low_pass_rad_s = entering_mode
			? input_.flight.pitch_rate_rad_s
			: first_order({ state_.pitch_rate_low_pass_rad_s,
				input_.flight.pitch_rate_rad_s,
				config_.pitch_rate_washout_time_constant_s,
				input_.flight.dt_s });
		const double washed_q = input_.flight.pitch_rate_rad_s -
			state_.pitch_rate_low_pass_rad_s;
		const double kp = Systems::fbw_blend_value(
			config_.normal_acceleration_proportional_cat1,
			config_.normal_acceleration_proportional_cat3,
			input_.configuration.stores_transition_0_1) *
			input_.configuration.gains.command_gain;
		const FeedbackEffort feedback = {
			kp * error_g,
			-config_.pitch_rate_feedback_gain_s * washed_q *
				input_.configuration.gains.damping_gain,
			alpha_feedback_effort() };
		return finish_normal_acceleration(
			{ requested_g, load, alpha, error_g, feedback });
	}

	Systems::LongitudinalAxisResult finish_normal_acceleration(
		const NormalAccelerationResultInput& input)
	{
		const double non_integral = input.feedback.normal_acceleration +
			input.feedback.pitch_rate + input.feedback.angle_of_attack;
		const bool transition = prepare_mode(
			Core::Systems::LongitudinalCommandMode::NormalAcceleration,
			non_integral);
		const double integral =
			state_.normal_acceleration_integral_effort;
		const double raw = non_integral + integral;
		const double effort = Common::limit(raw, -1.0, 1.0);
		const double protected_effort = Common::limit(
			non_integral, -1.0, 1.0);
		const TrackingInput tracking = {
			raw, effort, protected_effort, input.alpha.command_limited };
		const double correction = tracking_correction(tracking);
		update_normal_integral(input.error_g, tracking);
		const bool electronic_saturated =
			std::fabs(raw - effort) > kEffortDifferenceTolerance;
		return { effort,
			{ input.alpha.command_limited, input.load.limited, false,
				std::fabs(correction) > kEffortDifferenceTolerance,
				transition, electronic_saturated },
			{ Core::Systems::LongitudinalCommandMode::NormalAcceleration,
				input.requested_g, input.alpha.protected_g,
				input.alpha.command_decrement_g,
				0.0, 0.0, input.alpha.alpha_blend_0_1, input.alpha.maximum_g,
				input.error_g, 0.0, input.feedback.pitch_rate,
				input.feedback.angle_of_attack, integral, raw, effort } };
	}

	Systems::LongitudinalAxisResult update_pitch_rate()
	{
		const double requested = std::get<Core::Systems::PitchRateCommand>(
			input_.maneuver.longitudinal.command).target_rad_s;
		const double limit = input_.configuration.envelope.hard_protection
			.pitch_rate_limit_rad_s * input_.configuration.gains.limiter_gain;
		const double rate_limited = Common::limit(requested, -limit, limit);
		const auto alpha = Systems::protect_pitch_rate_command(
			{ rate_limited, alpha_schedule() });
		const bool body_rate_limited =
			std::fabs(rate_limited - requested) > kCommandDifferenceTolerance;
		return synthesize_pitch_rate(requested, alpha, body_rate_limited);
	}

	Systems::LongitudinalAxisResult synthesize_pitch_rate(
		double requested_rad_s,
		const Systems::PitchRateProtectionResult& alpha,
		bool body_rate_limited)
	{
		state_.pitch_rate_low_pass_rad_s = input_.flight.pitch_rate_rad_s;
		const double error =
			alpha.protected_rad_s - input_.flight.pitch_rate_rad_s;
		// F-16XL gear-down pitch-rate control blends alpha feedback in only
		// through the high-alpha schedule. Applying full static-alpha feedback
		// below the onset would fight ordinary landing/manual pitch commands.
		const double feedback = alpha_feedback_effort(alpha.alpha_blend_0_1);
		const double proportional =
			config_.pitch_rate_command_proportional_s * error *
			input_.configuration.gains.command_gain;
		const bool transition = prepare_mode(
			Core::Systems::LongitudinalCommandMode::PitchRate,
			proportional + feedback);
		const double integral = state_.pitch_rate_integral_effort;
		const double raw = proportional + feedback + integral;
		const double effort = Common::limit(raw, -1.0, 1.0);
		const double protected_effort = Common::limit(
			proportional + feedback, -1.0, 1.0);
		const TrackingInput tracking = {
			raw, effort, protected_effort, alpha.command_limited };
		const double correction = tracking_correction(tracking);
		update_pitch_integral(error, tracking);
		const bool electronic_saturated =
			std::fabs(raw - effort) > kEffortDifferenceTolerance;
		return { effort,
			{ alpha.command_limited, false, body_rate_limited,
				std::fabs(correction) > kEffortDifferenceTolerance,
				transition, electronic_saturated },
			{ Core::Systems::LongitudinalCommandMode::PitchRate,
				1.0, 1.0, 0.0, requested_rad_s,
				alpha.protected_rad_s, alpha.alpha_blend_0_1, 0.0,
				0.0, error, 0.0, feedback, integral, raw, effort } };
	}

	Systems::LongitudinalControlLawState& state_;
	const Systems::LongitudinalControlConfig& config_;
	const double stabilator_limit_rad_;
	const Systems::FlightControlLawsInput& input_;
};
}

namespace Systems
{
LongitudinalAxisResult update_longitudinal_control_law(
	LongitudinalControlLawState& state,
	const FlightControlLawsConfig& config,
	const FlightControlLawsInput& input)
{
	return LongitudinalFrame(state, config, input).run();
}
}
