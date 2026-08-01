#include "AutomaticFlightControl.h"

#include "Common/Clamp.h"
#include "Common/Units.h"

#include <cmath>

namespace
{
constexpr double kMinimumNormalizedCommand = -1.0;
constexpr double kMaximumNormalizedCommand = 1.0;
constexpr double kMinimumThrottleCommand = 0.0;
constexpr double kVerticalSpeedIntegralLimit = 5.0;
constexpr double kAltitudeVerticalSpeedIntegralLimit = 3.0;
constexpr double kHeadingIntegralLimit = 0.5;
constexpr double kSpeedIntegralLimit = 30.0;
constexpr double kFineAltitudeGain = 0.15;
constexpr double kHoldAltitudeGain = 0.5;
constexpr double kCaptureAltitudeGain = 1.5;
constexpr double kApproachAltitudeGain = 0.8;
constexpr double kCaptureMinimumVerticalSpeedMps = 2.0;
constexpr double kCaptureComfortDistanceFactor = 0.8;
constexpr double kApproachComfortDistanceFactor = 0.6;
constexpr double kTwo = 2.0;

double wrap_pi(double angle)
{
	const double period = kTwo * Common::kPi;
	while (angle > Common::kPi) angle -= period;
	while (angle < -Common::kPi) angle += period;
	return angle;
}

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
void AutomaticFlightControl::update_vertical_controller()
{
	if (!master_engaged_)
	{
		pitch_command_ = 0.0;
		return;
	}
	switch (vertical_mode_)
	{
	case AutomaticFlightControlVerticalMode::PitchHold:
		update_pitch_hold();
		break;
	case AutomaticFlightControlVerticalMode::VerticalSpeedHold:
		update_vertical_speed_hold();
		break;
	case AutomaticFlightControlVerticalMode::AltitudeHold:
		update_altitude_hold();
		break;
	default:
		pitch_command_ = 0.0;
		break;
	}
}

void AutomaticFlightControl::update_lateral_controller()
{
	if (!master_engaged_)
	{
		roll_command_ = 0.0;
		return;
	}
	if (lateral_mode_ == AutomaticFlightControlLateralMode::HeadingHold)
		update_heading_control(target_heading_rad_);
	else if (lateral_mode_ == AutomaticFlightControlLateralMode::HeadingSelect)
		update_heading_control(target_heading_select_rad_);
	else
		roll_command_ = 0.0;
}

void AutomaticFlightControl::update_pitch_hold()
{
	const double error = target_pitch_rad_ - observation_.pitch_rad;
	const double command = config_.pitch_kp * error -
		config_.pitch_kd *
			observation_.legacy_pitch_damping_rate_rad_s;
	pitch_command_ = Common::limit(
		command,
		-config_.pitch_command_limit,
		config_.pitch_command_limit);
}

void AutomaticFlightControl::update_vertical_speed_hold()
{
	const double error =
		target_vertical_speed_mps_ - observation_.vertical_speed_mps;
	vertical_speed_integral_ = Common::limit(
		vertical_speed_integral_ + error * observation_.dt_s,
		-kVerticalSpeedIntegralLimit,
		kVerticalSpeedIntegralLimit);
	const double command = config_.vertical_speed_kp * error +
		config_.vertical_speed_ki * vertical_speed_integral_;
	pitch_command_ = Common::limit(
		command,
		-config_.pitch_command_limit,
		config_.pitch_command_limit);
}

void AutomaticFlightControl::update_altitude_hold()
{
	const double altitude_error =
		target_altitude_m_ - observation_.altitude_m;
	const double desired_vertical_speed =
		altitude_desired_vertical_speed(altitude_error);
	const double error =
		desired_vertical_speed - observation_.vertical_speed_mps;
	vertical_speed_integral_ = Common::limit(
		vertical_speed_integral_ + error * observation_.dt_s,
		-kAltitudeVerticalSpeedIntegralLimit,
		kAltitudeVerticalSpeedIntegralLimit);
	const double command = config_.altitude_vertical_speed_kp * error +
		config_.altitude_vertical_speed_ki * vertical_speed_integral_;
	pitch_command_ = Common::limit(
		command,
		-config_.pitch_command_limit,
		config_.pitch_command_limit);
}

double AutomaticFlightControl::altitude_desired_vertical_speed(
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
			altitude_error_m,
			config_.vertical_speed_limit_mps,
			kApproachComfortDistanceFactor);
		return std::abs(comfort) < std::abs(desired) ? comfort : desired;
	}
	const double fraction =
		(absolute_error - config_.altitude_hold_band_m) /
		(config_.altitude_capture_band_m - config_.altitude_hold_band_m);
	const double maximum = config_.vertical_speed_capture_taper_mps *
		fraction + kCaptureMinimumVerticalSpeedMps;
	const double desired = Common::limit(
		kCaptureAltitudeGain * altitude_error_m, -maximum, maximum);
	if (std::abs(observation_.vertical_speed_mps) <=
		kCaptureMinimumVerticalSpeedMps)
		return desired;
	const double comfort = comfort_limited_vertical_speed(
		altitude_error_m, maximum, kCaptureComfortDistanceFactor);
	return std::abs(comfort) < std::abs(desired) ? comfort : desired;
}

double AutomaticFlightControl::comfort_limited_vertical_speed(
	double altitude_error_m,
	double maximum_vertical_speed_mps,
	double remaining_distance_factor) const
{
	const double vertical_speed = observation_.vertical_speed_mps;
	const double remaining = std::abs(altitude_error_m);
	const double stop_distance = vertical_speed * vertical_speed /
		(kTwo * config_.altitude_comfort_acceleration_mps2);
	if (stop_distance <= remaining * remaining_distance_factor)
		return sign(altitude_error_m) * maximum_vertical_speed_mps;
	const double comfortable = sign(altitude_error_m) * std::sqrt(
		kTwo * config_.altitude_comfort_acceleration_mps2 *
		remaining * remaining_distance_factor);
	return Common::limit(
		comfortable,
		-maximum_vertical_speed_mps,
		maximum_vertical_speed_mps);
}

void AutomaticFlightControl::update_heading_control(double target_heading_rad)
{
	const double heading_error =
		wrap_pi(target_heading_rad - observation_.heading_rad);
	heading_integral_ = Common::limit(
		heading_integral_ + heading_error * observation_.dt_s,
		-kHeadingIntegralLimit,
		kHeadingIntegralLimit);
	const double desired_bank = Common::limit(
		config_.heading_kp * heading_error +
			config_.heading_ki * heading_integral_,
		-config_.bank_limit_rad,
		config_.bank_limit_rad);
	const double bank_error = desired_bank - observation_.roll_rad;
	const double command = config_.bank_kp * bank_error -
		config_.bank_kd *
			observation_.legacy_heading_damping_rate_rad_s;
	roll_command_ = Common::limit(
		command,
		kMinimumNormalizedCommand,
		kMaximumNormalizedCommand);
}

void AutomaticFlightControl::update_auto_throttle()
{
	if (!auto_throttle_engaged_)
	{
		throttle_command_ = 0.0;
		return;
	}
	if (observation_.mach > config_.maximum_auto_throttle_mach)
	{
		throttle_command_ = Common::limit(
			throttle_command_ -
				config_.mach_guard_throttle_rate_per_s * observation_.dt_s,
			kMinimumThrottleCommand,
			kMaximumNormalizedCommand);
		return;
	}
	const double error =
		target_speed_mps_ - observation_.indicated_airspeed_mps;
	speed_integral_ = Common::limit(
		speed_integral_ + error * observation_.dt_s,
		-kSpeedIntegralLimit,
		kSpeedIntegralLimit);
	const double command = config_.auto_throttle_base +
		config_.auto_throttle_kp * error +
		config_.auto_throttle_ki * speed_integral_;
	throttle_command_ = Common::limit(
		command,
		kMinimumThrottleCommand,
		config_.auto_throttle_command_limit);
}
}
}
