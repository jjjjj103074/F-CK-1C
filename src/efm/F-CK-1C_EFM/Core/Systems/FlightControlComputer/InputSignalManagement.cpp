#include "InputSignalManagement.h"

#include "ControlLaws/ControlLawMath.h"
#include "Common/Clamp.h"

#include <cmath>
#include <stdexcept>

namespace
{
constexpr double kMinimumTimeConstantS = 1e-6;
constexpr double kNormalizedMinimum = -1.0;
constexpr double kNormalizedMaximum = 1.0;

bool finite(double value)
{
	return std::isfinite(value);
}

void require_finite(double value, const char* field)
{
	if (!finite(value))
	{
		throw std::invalid_argument(field);
	}
}

void require_normalized(double value, const char* field)
{
	require_finite(value, field);
	if (value < kNormalizedMinimum || value > kNormalizedMaximum)
	{
		throw std::out_of_range(field);
	}
}

void require_unit_interval(double value, const char* field)
{
	require_finite(value, field);
	if (value < 0.0 || value > 1.0)
	{
		throw std::out_of_range(field);
	}
}

double wrap_pi(double angle_rad)
{
	return std::atan2(std::sin(angle_rad), std::cos(angle_rad));
}
}

namespace Core
{
namespace Systems
{
InputSignalManagement::InputSignalManagement(
	const ::Systems::FBWControllerConfig& config)
	: config_(config)
{
}

::Systems::ConditionedFlightControlInput InputSignalManagement::condition(
	const RawFlightControlInput& raw,
	::Systems::FBWCatMode cat_mode)
{
	validate(raw);
	const double target_blend =
		cat_mode == ::Systems::FBW_CAT3 ? 1.0 : 0.0;
	state_.cat_mode_blend = filter(
		state_.cat_mode_blend,
		target_blend,
		config_.mode_switch_tau,
		raw.dt_s);
	const ::Systems::FBWCatParams cat = ::Systems::fbw_blend_cat_params(
		config_.cat1, config_.cat3, state_.cat_mode_blend);
	state_.dt_s = raw.dt_s;
	state_.alpha_limit_deg = raw.alpha_limit_deg;
	update_observation(state_, raw);
	update_pilot_signal(state_, raw.pilot, cat);
	update_actuator_feedback(state_, raw);
	return state_;
}

double InputSignalManagement::cat_mode_blend() const
{
	return state_.cat_mode_blend;
}

double InputSignalManagement::filter(
	double current,
	double target,
	double tau_s,
	double dt_s) const
{
	if (tau_s <= kMinimumTimeConstantS)
	{
		return target;
	}
	const double gain = Common::limit(dt_s / (tau_s + dt_s), 0.0, 1.0);
	return current + (target - current) * gain;
}

double InputSignalManagement::shape_stick(
	double value,
	double exponent_weight) const
{
	return (1.0 - exponent_weight) * value +
		exponent_weight * value * value * value;
}

double InputSignalManagement::filter_angle(
	double current_rad,
	double target_rad,
	double tau_s,
	double dt_s) const
{
	const double delta_rad = wrap_pi(target_rad - current_rad);
	return wrap_pi(filter(current_rad, current_rad + delta_rad, tau_s, dt_s));
}

void InputSignalManagement::validate(const RawFlightControlInput& raw) const
{
	require_finite(raw.dt_s, "flight-control dt");
	if (raw.dt_s <= 0.0)
	{
		throw std::out_of_range("flight-control dt");
	}
	require_finite(raw.alpha_limit_deg, "alpha limit");
	const double observations[] = {
		raw.observation.altitude_asl_m,
		raw.observation.indicated_airspeed_mps,
		raw.observation.vertical_speed_mps,
		raw.observation.mach,
		raw.observation.dynamic_pressure_pa,
		raw.observation.normal_acceleration_g,
		raw.observation.alpha_deg,
		raw.observation.beta_deg,
		raw.observation.heading_rad,
		raw.observation.roll_rad,
		raw.observation.pitch_rad,
		raw.observation.roll_rate_rad_s,
		raw.observation.pitch_rate_rad_s,
		raw.observation.yaw_rate_rad_s
	};
	for (double value : observations)
	{
		require_finite(value, "flight-control observation");
	}
	require_normalized(raw.pilot.pitch_axis_normalized, "pitch axis");
	require_normalized(raw.pilot.roll_axis_normalized, "roll axis");
	require_normalized(raw.pilot.yaw_axis_normalized, "yaw axis");
	require_normalized(raw.pilot.pitch_trim_normalized, "pitch trim");
	require_normalized(raw.pilot.roll_trim_normalized, "roll trim");
	require_normalized(raw.pilot.yaw_trim_normalized, "yaw trim");
	require_unit_interval(raw.landing_gear.position, "landing-gear position");
	const FlightControlSurfaceState* surfaces[] = {
		&raw.actuator.elevator,
		&raw.actuator.aileron,
		&raw.actuator.rudder
	};
	for (const FlightControlSurfaceState* surface : surfaces)
	{
		require_finite(surface->position_rad, "actuator position");
		require_finite(surface->rate_rad_s, "actuator rate");
		require_normalized(
			surface->normalized_position, "normalized actuator position");
	}
}

void InputSignalManagement::update_observation(
	::Systems::ConditionedFlightControlInput& output,
	const RawFlightControlInput& raw)
{
	const FlightControlObservation& source = raw.observation;
	const double dt_s = raw.dt_s;
	const double tau_s = config_.signal_filter_tau;
	output.altitude_asl_m = filter(
		output.altitude_asl_m, source.altitude_asl_m, tau_s, dt_s);
	output.vertical_speed_mps = filter(
		output.vertical_speed_mps, source.vertical_speed_mps, tau_s, dt_s);
	output.heading_rad = filter_angle(
		output.heading_rad, source.heading_rad, tau_s, dt_s);
	output.roll_attitude_rad = filter_angle(
		output.roll_attitude_rad, source.roll_rad, tau_s, dt_s);
	output.pitch_attitude_rad = filter(
		output.pitch_attitude_rad, source.pitch_rad, tau_s, dt_s);
	output.roll_rate_rad_s = filter(
		output.roll_rate_rad_s, source.roll_rate_rad_s, tau_s, dt_s);
	output.pitch_rate_rad_s = filter(
		output.pitch_rate_rad_s, source.pitch_rate_rad_s, tau_s, dt_s);
	output.yaw_rate_rad_s = filter(
		output.yaw_rate_rad_s, source.yaw_rate_rad_s, tau_s, dt_s);
	output.angle_of_attack_deg = filter(
		output.angle_of_attack_deg, source.alpha_deg, tau_s, dt_s);
	output.sideslip_deg = filter(
		output.sideslip_deg, source.beta_deg, tau_s, dt_s);
	output.dynamic_pressure_pa = filter(
		output.dynamic_pressure_pa,
		source.dynamic_pressure_pa,
		config_.qbar_filter_tau,
		dt_s);
	output.indicated_airspeed_mps = filter(
		output.indicated_airspeed_mps,
		source.indicated_airspeed_mps,
		tau_s,
		dt_s);
	output.mach = filter(output.mach, source.mach, tau_s, dt_s);
	output.normal_acceleration_g = filter(
		output.normal_acceleration_g,
		source.normal_acceleration_g,
		config_.nz_filter_tau,
		dt_s);
}

void InputSignalManagement::update_pilot_signal(
	::Systems::ConditionedFlightControlInput& output,
	const PilotControlSignal& pilot,
	const ::Systems::FBWCatParams& cat)
{
	output.pilot_roll_raw_normalized = Common::limit(
		pilot.roll_axis_normalized + pilot.roll_trim_normalized, -1.0, 1.0);
	output.pilot_pitch_raw_normalized = Common::limit(
		pilot.pitch_axis_normalized + pilot.pitch_trim_normalized, -1.0, 1.0);
	output.pilot_yaw_raw_normalized = Common::limit(
		pilot.yaw_axis_normalized + pilot.yaw_trim_normalized, -1.0, 1.0);
	const double roll_target = shape_stick(
		output.pilot_roll_raw_normalized, cat.stick_expo);
	const double pitch_target = shape_stick(
		output.pilot_pitch_raw_normalized, cat.stick_expo);
	const double yaw_target = shape_stick(
		output.pilot_yaw_raw_normalized, cat.stick_expo);
	const double maximum_step = cat.command_shape_rate * output.dt_s;
	shaped_roll_normalized_ = Common::limit(
		filter(shaped_roll_normalized_, roll_target, cat.command_shape_tau, output.dt_s),
		shaped_roll_normalized_ - maximum_step,
		shaped_roll_normalized_ + maximum_step);
	shaped_pitch_normalized_ = Common::limit(
		filter(shaped_pitch_normalized_, pitch_target, cat.command_shape_tau, output.dt_s),
		shaped_pitch_normalized_ - maximum_step,
		shaped_pitch_normalized_ + maximum_step);
	shaped_yaw_normalized_ = Common::limit(
		filter(shaped_yaw_normalized_, yaw_target, cat.command_shape_tau, output.dt_s),
		shaped_yaw_normalized_ - maximum_step,
		shaped_yaw_normalized_ + maximum_step);
	output.pilot_roll_normalized = shaped_roll_normalized_;
	output.pilot_pitch_normalized = shaped_pitch_normalized_;
	output.pilot_yaw_normalized = shaped_yaw_normalized_;
	output.roll_pitch_in_deadband =
		std::fabs(output.pilot_roll_raw_normalized) <= cat.deadband &&
		std::fabs(output.pilot_pitch_raw_normalized) <= cat.deadband;
	output.pitch_in_deadband =
		std::fabs(output.pilot_pitch_raw_normalized) <= cat.deadband;
}

void InputSignalManagement::update_actuator_feedback(
	::Systems::ConditionedFlightControlInput& output,
	const RawFlightControlInput& raw) const
{
	output.gear_position_normalized = raw.landing_gear.position;
	output.weight_on_wheels = raw.landing_gear.any_weight_on_wheels;
	output.elevator_position_normalized =
		raw.actuator.elevator.normalized_position;
	output.aileron_position_normalized =
		raw.actuator.aileron.normalized_position;
	output.rudder_position_normalized =
		raw.actuator.rudder.normalized_position;
	output.actuator_saturated = raw.actuator.any_saturated;
}
}
}
