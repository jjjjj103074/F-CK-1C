#include "ControlLaws.h"

#include "Internal/ControlLawMath.h"
#include "Internal/InnerLoopControl.h"
#include "Internal/LongitudinalControlLaw.h"
#include "Common/Clamp.h"

#include <cmath>

namespace
{
constexpr double kRateConstraintToleranceRadS = 1.0e-5;

struct AxisLawResult
{
	double effort_normalized = 0.0;
	bool rate_limited = false;
	bool anti_windup_active = false;
};

struct LateralDirectionalReference
{
	double roll_rate_rad_s = 0.0;
	double yaw_rate_rad_s = 0.0;
};

struct SurfaceControlEffort
{
	double longitudinal = 0.0;
	double lateral = 0.0;
	double directional = 0.0;
};

struct AxisGainsInput
{
	double proportional = 0.0;
	double integral = 0.0;
	const Systems::FlightControlLawsInput& laws;
	const Systems::InnerRateControlConfig& config;
};


Systems::AxisRateLoopGains axis_gains(const AxisGainsInput& input)
{
	const double damping = input.laws.configuration.gains.damping_gain;
	return { input.proportional * damping, input.integral * damping,
		input.config.anti_windup_gain, input.config.integral_limit };
}

AxisLawResult update_rate_axis(
	double& integral,
	const Systems::AxisRateLoopStepInput& step,
	double limit_rad_s)
{
	const double limited_reference = Common::limit(
		step.reference_rad_s, -limit_rad_s, limit_rad_s);
	Systems::AxisRateLoopStepInput limited = step;
	limited.reference_rad_s = limited_reference;
	const Systems::AxisRateLoopResult result =
		Systems::update_axis_rate_loop(integral, limited);
	integral = result.integral;
	return { result.effort_normalized,
		std::fabs(limited_reference - step.reference_rad_s) >
			kRateConstraintToleranceRadS,
		result.anti_windup_active };
}

class LateralDirectionalCoordination
{
public:
	static LateralDirectionalReference update(
		const Systems::FlightControlLawsInput& input)
	{
		const auto& reference = input.maneuver.lateral_directional;
		const auto& schedule = input.configuration.stores.directional;
		const double sideslip_error_rad =
			input.flight.sideslip_rad - reference.sideslip_reference_rad;
		const double damping_rad_s = -(
			schedule.sideslip_damping_s_inv * sideslip_error_rad +
			schedule.yaw_rate_damping * input.flight.yaw_rate_rad_s) *
			input.configuration.gains.damping_gain;
		return { reference.roll_rate_reference_rad_s,
			reference.yaw_rate_feedforward_rad_s + damping_rad_s };
	}
};

class LateralControlLaw
{
public:
	LateralControlLaw(
		Systems::LateralControlLawState& state,
		const Systems::FlightControlLawsConfig& config)
		: state_(state), config_(config) {}

	AxisLawResult update(
		const Systems::FlightControlLawsInput& input,
		double reference_rad_s)
	{
		const auto& inner = config_.inner_rate;
		const double limit = input.configuration.envelope.hard_protection
			.roll_rate_limit_rad_s * input.configuration.gains.limiter_gain;
		return update_rate_axis(state_.roll_integral,
			{ input.flight.dt_s, reference_rad_s,
				input.flight.roll_rate_rad_s,
				axis_gains({ inner.roll_proportional, inner.roll_integral,
					input, inner }) }, limit);
	}

private:
	Systems::LateralControlLawState& state_;
	const Systems::FlightControlLawsConfig& config_;
};

class DirectionalControlLaw
{
public:
	DirectionalControlLaw(
		Systems::DirectionalControlLawState& state,
		const Systems::FlightControlLawsConfig& config)
		: state_(state), config_(config) {}

	AxisLawResult update(
		const Systems::FlightControlLawsInput& input,
		double reference_rad_s)
	{
		const auto& inner = config_.inner_rate;
		const double limit = input.configuration.envelope.hard_protection
			.yaw_rate_limit_rad_s * input.configuration.gains.limiter_gain;
		return update_rate_axis(state_.yaw_integral,
			{ input.flight.dt_s, reference_rad_s,
				input.flight.yaw_rate_rad_s,
				axis_gains({ inner.yaw_proportional, inner.yaw_integral,
					input, inner }) }, limit);
	}

private:
	Systems::DirectionalControlLawState& state_;
	const Systems::FlightControlLawsConfig& config_;
};

Systems::ControlSurfaceDemandSet mix_surface_commands(
	const SurfaceControlEffort& effort,
	const Systems::SurfaceCommandMixerConfig& config)
{
	return {
		effort.longitudinal * config.symmetric_stabilator_limit_rad,
		effort.lateral * config.differential_flaperon_limit_rad,
		effort.directional * config.rudder_limit_rad
	};
}

Systems::ControlSurfaceDemandSet direct_surface_commands(
	const Systems::FlightControlLawsInput& input,
	const Systems::SurfaceCommandMixerConfig& config)
{
	return {
		input.signals.pilot_pitch_raw_normalized *
			config.symmetric_stabilator_limit_rad,
		input.signals.pilot_roll_raw_normalized *
			config.differential_flaperon_limit_rad,
		input.signals.pilot_yaw_raw_normalized * config.rudder_limit_rad
	};
}
}

namespace Systems
{
FlightControlLaws::FlightControlLaws(const FlightControlLawsConfig& config)
	: config_(config)
{
}

FlightControlLawsResult FlightControlLaws::update(
	const FlightControlLawsInput& input)
{
	const LongitudinalAxisResult pitch = update_longitudinal_control_law(
		longitudinal_, config_, input);
	const LateralDirectionalReference coordinated =
		LateralDirectionalCoordination::update(input);
	const AxisLawResult roll =
		LateralControlLaw(lateral_, config_).update(
			input, coordinated.roll_rate_rad_s);
	const AxisLawResult yaw =
		DirectionalControlLaw(directional_, config_).update(
			input, coordinated.yaw_rate_rad_s);
	FlightControlLawsStatus status = pitch.status;
	status.body_rate_limit_active = status.body_rate_limit_active ||
		roll.rate_limited || yaw.rate_limited;
	status.anti_windup_active = status.anti_windup_active ||
		roll.anti_windup_active || yaw.anti_windup_active;
	status.electronic_command_saturated =
		status.electronic_command_saturated ||
		roll.anti_windup_active || yaw.anti_windup_active;
	return {
		mix_surface_commands({ pitch.effort_normalized,
			roll.effort_normalized, yaw.effort_normalized },
			config_.surface_mixer),
		direct_surface_commands(input, config_.surface_mixer),
		status,
		pitch.diagnostics
	};
}

void FlightControlLaws::reset()
{
	longitudinal_ = {};
	lateral_ = {};
	directional_ = {};
}
}
