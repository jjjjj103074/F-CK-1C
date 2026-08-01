#include "VerticalGuidance.h"

#include "Common/Clamp.h"

#include <cmath>

namespace
{
constexpr double kTwo = 2.0;

double sign(double value)
{
	if (value > 0.0) return 1.0;
	if (value < 0.0) return -1.0;
	return 0.0;
}
}

namespace Core
{
namespace Systems
{
VerticalGuidance::VerticalGuidance(
	const AutomaticFlightControlConfig& config)
	: config_(config)
{
}

VerticalGuidanceReference VerticalGuidance::update(
	const AutomaticFlightControlObservation& observation,
	const VerticalGuidanceTarget& target,
	bool active)
{
	if (!active)
	{
		reset();
		return {};
	}
	if (target.mode != previous_mode_)
	{
		pitch_reference_rad_ = observation.pitch_rad;
		vertical_speed_reference_mps_ = observation.vertical_speed_mps;
		previous_mode_ = target.mode;
	}
	switch (target.mode)
	{
	case AutomaticFlightControlVerticalMode::PitchHold:
		return update_pitch_reference(observation, target.pitch_attitude_rad);
	case AutomaticFlightControlVerticalMode::VerticalSpeedHold:
		return update_vertical_speed_reference(
			observation, target.vertical_speed_mps);
	case AutomaticFlightControlVerticalMode::AltitudeHold:
		return update_altitude_reference(observation, target.altitude_m);
	default:
		reset();
		return {};
	}
}

VerticalGuidanceReference VerticalGuidance::update_pitch_reference(
	const AutomaticFlightControlObservation& observation,
	double target_pitch_rad)
{
	pitch_reference_rad_ = rate_limit({
		pitch_reference_rad_,
		target_pitch_rad,
		config_.pitch_reference_rate_rad_s,
		observation.dt_s
	});
	return {
		VerticalGuidanceReferenceType::PitchAttitude,
		pitch_reference_rad_,
		0.0
	};
}

VerticalGuidanceReference VerticalGuidance::update_vertical_speed_reference(
	const AutomaticFlightControlObservation& observation,
	double target_vertical_speed_mps)
{
	vertical_speed_reference_mps_ = rate_limit({
		vertical_speed_reference_mps_,
		target_vertical_speed_mps,
		config_.vertical_reference_acceleration_mps2,
		observation.dt_s
	});
	return {
		VerticalGuidanceReferenceType::VerticalSpeed,
		0.0,
		vertical_speed_reference_mps_
	};
}

VerticalGuidanceReference VerticalGuidance::update_altitude_reference(
	const AutomaticFlightControlObservation& observation,
	double target_altitude_m)
{
	const double target_vertical_speed_mps =
		altitude_desired_vertical_speed(
			observation, target_altitude_m - observation.altitude_m);
	return update_vertical_speed_reference(
		observation, target_vertical_speed_mps);
}

void VerticalGuidance::reset()
{
	previous_mode_ = AutomaticFlightControlVerticalMode::Off;
}

double VerticalGuidance::rate_limit(const RateLimitInput& input) const
{
	const double maximum_step = input.maximum_rate_per_s * input.dt_s;
	return Common::limit(
		input.target,
		input.current - maximum_step,
		input.current + maximum_step);
}

double VerticalGuidance::altitude_desired_vertical_speed(
	const AutomaticFlightControlObservation& observation,
	double altitude_error_m) const
{
	const double absolute_error = std::abs(altitude_error_m);
	if (absolute_error <= config_.altitude_fine_band_m)
		return config_.altitude_fine_gain_s_inv * altitude_error_m;
	if (absolute_error <= config_.altitude_hold_band_m)
		return config_.altitude_hold_gain_s_inv * altitude_error_m;
	if (absolute_error > config_.altitude_capture_band_m)
	{
		const double desired = Common::limit(
			config_.altitude_approach_gain_s_inv * altitude_error_m,
			-config_.vertical_speed_limit_mps,
			config_.vertical_speed_limit_mps);
		const double comfort = comfort_limited_vertical_speed(
			observation,
			{ altitude_error_m,
				config_.vertical_speed_limit_mps,
				config_.approach_comfort_distance_factor });
		return std::abs(comfort) < std::abs(desired) ? comfort : desired;
	}
	const double fraction =
		(absolute_error - config_.altitude_hold_band_m) /
		(config_.altitude_capture_band_m - config_.altitude_hold_band_m);
	const double maximum =
		config_.vertical_speed_capture_taper_mps * fraction +
		config_.capture_minimum_vertical_speed_mps;
	const double desired = Common::limit(
		config_.altitude_capture_gain_s_inv * altitude_error_m,
		-maximum,
		maximum);
	if (std::abs(observation.vertical_speed_mps) <=
		config_.capture_minimum_vertical_speed_mps)
	{
		return desired;
	}
	const double comfort = comfort_limited_vertical_speed(
		observation,
		{ altitude_error_m,
			maximum,
			config_.capture_comfort_distance_factor });
	return std::abs(comfort) < std::abs(desired) ? comfort : desired;
}

double VerticalGuidance::comfort_limited_vertical_speed(
	const AutomaticFlightControlObservation& observation,
	const ComfortLimitInput& input) const
{
	const double vertical_speed = observation.vertical_speed_mps;
	const double remaining = std::abs(input.altitude_error_m);
	const double stop_distance = vertical_speed * vertical_speed /
		(kTwo * config_.altitude_comfort_acceleration_mps2);
	if (stop_distance <= remaining * input.remaining_distance_factor)
	{
		return sign(input.altitude_error_m) *
			input.maximum_vertical_speed_mps;
	}
	const double comfortable = sign(input.altitude_error_m) * std::sqrt(
		kTwo * config_.altitude_comfort_acceleration_mps2 *
		remaining * input.remaining_distance_factor);
	return Common::limit(
		comfortable,
		-input.maximum_vertical_speed_mps,
		input.maximum_vertical_speed_mps);
}
}
}
