#include "VerticalGuidance.h"

#include "Common/Clamp.h"

#include <cmath>

namespace
{
constexpr double kVerticalSpeedIntegralLimit = 5.0;
constexpr double kAltitudeVerticalSpeedIntegralLimit = 3.0;
constexpr double kFineAltitudeGain = 0.15;
constexpr double kHoldAltitudeGain = 0.5;
constexpr double kCaptureAltitudeGain = 1.5;
constexpr double kApproachAltitudeGain = 0.8;
constexpr double kCaptureMinimumVerticalSpeedMps = 2.0;
constexpr double kCaptureComfortDistanceFactor = 0.8;
constexpr double kApproachComfortDistanceFactor = 0.6;
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

double VerticalGuidance::update(
	const AutomaticFlightControlObservation& observation,
	const VerticalGuidanceTarget& target,
	bool active)
{
	if (!active)
	{
		return 0.0;
	}
	switch (target.mode)
	{
	case AutomaticFlightControlVerticalMode::PitchHold:
		return update_pitch_hold(observation, target.pitch_attitude_rad);
	case AutomaticFlightControlVerticalMode::VerticalSpeedHold:
		return update_vertical_speed_hold(
			observation, target.vertical_speed_mps);
	case AutomaticFlightControlVerticalMode::AltitudeHold:
		return update_altitude_hold(observation, target.altitude_m);
	default:
		return 0.0;
	}
}

void VerticalGuidance::reset()
{
	vertical_speed_integral_ = 0.0;
}

double VerticalGuidance::update_pitch_hold(
	const AutomaticFlightControlObservation& observation,
	double target_pitch_rad) const
{
	const double error = target_pitch_rad - observation.pitch_rad;
	const double command = config_.pitch_kp * error -
		config_.pitch_kd * observation.legacy_pitch_damping_rate_rad_s;
	return Common::limit(
		command, -config_.pitch_command_limit, config_.pitch_command_limit);
}

double VerticalGuidance::update_vertical_speed_hold(
	const AutomaticFlightControlObservation& observation,
	double target_vertical_speed_mps)
{
	const double error =
		target_vertical_speed_mps - observation.vertical_speed_mps;
	vertical_speed_integral_ = Common::limit(
		vertical_speed_integral_ + error * observation.dt_s,
		-kVerticalSpeedIntegralLimit,
		kVerticalSpeedIntegralLimit);
	const double command = config_.vertical_speed_kp * error +
		config_.vertical_speed_ki * vertical_speed_integral_;
	return Common::limit(
		command, -config_.pitch_command_limit, config_.pitch_command_limit);
}

double VerticalGuidance::update_altitude_hold(
	const AutomaticFlightControlObservation& observation,
	double target_altitude_m)
{
	const double altitude_error = target_altitude_m - observation.altitude_m;
	const double desired_vertical_speed =
		altitude_desired_vertical_speed(observation, altitude_error);
	const double error = desired_vertical_speed - observation.vertical_speed_mps;
	vertical_speed_integral_ = Common::limit(
		vertical_speed_integral_ + error * observation.dt_s,
		-kAltitudeVerticalSpeedIntegralLimit,
		kAltitudeVerticalSpeedIntegralLimit);
	const double command = config_.altitude_vertical_speed_kp * error +
		config_.altitude_vertical_speed_ki * vertical_speed_integral_;
	return Common::limit(
		command, -config_.pitch_command_limit, config_.pitch_command_limit);
}

double VerticalGuidance::altitude_desired_vertical_speed(
	const AutomaticFlightControlObservation& observation,
	double altitude_error_m) const
{
	const double absolute_error = std::abs(altitude_error_m);
	if (absolute_error <= config_.altitude_fine_band_m)
		return kFineAltitudeGain * altitude_error_m;
	if (absolute_error <= config_.altitude_hold_band_m)
		return kHoldAltitudeGain * altitude_error_m;
	if (absolute_error > config_.altitude_capture_band_m)
	{
		const double desired = Common::limit(
			kApproachAltitudeGain * altitude_error_m,
			-config_.vertical_speed_limit_mps,
			config_.vertical_speed_limit_mps);
		const double comfort = comfort_limited_vertical_speed(
			observation,
			altitude_error_m,
			config_.vertical_speed_limit_mps,
			kApproachComfortDistanceFactor);
		return std::abs(comfort) < std::abs(desired) ? comfort : desired;
	}
	const double fraction =
		(absolute_error - config_.altitude_hold_band_m) /
		(config_.altitude_capture_band_m - config_.altitude_hold_band_m);
	const double maximum =
		config_.vertical_speed_capture_taper_mps * fraction +
		kCaptureMinimumVerticalSpeedMps;
	const double desired = Common::limit(
		kCaptureAltitudeGain * altitude_error_m, -maximum, maximum);
	if (std::abs(observation.vertical_speed_mps) <=
		kCaptureMinimumVerticalSpeedMps)
	{
		return desired;
	}
	const double comfort = comfort_limited_vertical_speed(
		observation,
		altitude_error_m,
		maximum,
		kCaptureComfortDistanceFactor);
	return std::abs(comfort) < std::abs(desired) ? comfort : desired;
}

double VerticalGuidance::comfort_limited_vertical_speed(
	const AutomaticFlightControlObservation& observation,
	double altitude_error_m,
	double maximum_vertical_speed_mps,
	double remaining_distance_factor) const
{
	const double vertical_speed = observation.vertical_speed_mps;
	const double remaining = std::abs(altitude_error_m);
	const double stop_distance = vertical_speed * vertical_speed /
		(kTwo * config_.altitude_comfort_acceleration_mps2);
	if (stop_distance <= remaining * remaining_distance_factor)
	{
		return sign(altitude_error_m) * maximum_vertical_speed_mps;
	}
	const double comfortable = sign(altitude_error_m) * std::sqrt(
		kTwo * config_.altitude_comfort_acceleration_mps2 *
		remaining * remaining_distance_factor);
	return Common::limit(
		comfortable,
		-maximum_vertical_speed_mps,
		maximum_vertical_speed_mps);
}
}
}
