#include "AutomaticFlightControl.h"
#include "../ControlLaws/ControlLawConfig.h"

#include "Common/Units.h"

#include <cmath>
#include <stdexcept>
#include <string>

namespace
{
constexpr double kMetersPerFoot = 0.3048;
constexpr double kMetersPerSecondPerKnot = 0.5144444444444445;
constexpr double kGravityMps2 = 9.81;

void require_positive(double value, const char* name)
{
	if (!std::isfinite(value) || value <= 0.0)
	{
		throw std::invalid_argument(
			std::string("Invalid automatic flight control ") + name + ".");
	}
}

void validate_guidance_tuning(
	const Core::Systems::AutomaticFlightControlConfig& config)
{
	require_positive(config.minimum_ias_mps, "minimum IAS");
	require_positive(config.engage_roll_limit_rad, "roll engage limit");
	require_positive(config.engage_pitch_limit_rad, "pitch engage limit");
	require_positive(config.bank_limit_rad, "bank limit");
	require_positive(config.roll_reference_rate_rad_s, "roll reference rate");
	require_positive(config.pitch_reference_rate_rad_s, "pitch reference rate");
	require_positive(
		config.vertical_reference_acceleration_mps2,
		"vertical reference acceleration");
	require_positive(config.altitude_fine_band_m, "altitude fine band");
	require_positive(config.altitude_hold_band_m, "altitude hold band");
	require_positive(config.altitude_capture_band_m, "altitude capture band");
	require_positive(config.altitude_comfort_acceleration_mps2, "comfort acceleration");
	require_positive(config.altitude_fine_gain_s_inv, "fine altitude gain");
	require_positive(config.altitude_hold_gain_s_inv, "hold altitude gain");
	require_positive(config.altitude_capture_gain_s_inv, "capture altitude gain");
	require_positive(config.altitude_approach_gain_s_inv, "approach altitude gain");
	require_positive(
		config.capture_minimum_vertical_speed_mps,
		"capture minimum vertical speed");
	require_positive(config.vertical_speed_limit_mps, "vertical speed limit");
	require_positive(
		config.vertical_speed_capture_taper_mps,
		"vertical-speed capture taper");
}

void validate_reference_controls(
	const Core::Systems::AutomaticFlightControlConfig& config)
{
	require_positive(
		config.capture_comfort_distance_factor,
		"capture comfort distance factor");
	require_positive(
		config.approach_comfort_distance_factor,
		"approach comfort distance factor");
	require_positive(
		config.pitch_stick_steering_threshold_normalized,
		"pitch stick-steering threshold");
	require_positive(config.altitude_step_m, "altitude step");
	require_positive(config.heading_step_rad, "heading step");
	require_positive(config.pitch_step_rad, "pitch step");
	require_positive(config.vertical_speed_step_mps, "vertical-speed step");
	require_positive(config.heading_kp, "heading proportional gain");
	require_positive(config.heading_ki, "heading integral gain");
	require_positive(
		config.heading_error_integral_limit_rad_s,
		"heading-error integral limit");
}

void validate_monitor_and_auto_throttle(
	const Core::Systems::AutomaticFlightControlConfig& config)
{
	const auto& assist = config.experimental_auto_throttle;
	require_positive(assist.maximum_mach, "Mach guard");
	require_positive(assist.disconnect_mach, "Mach disconnect");
	require_positive(assist.speed_step_mps, "speed step");
	require_positive(assist.minimum_target_speed_mps, "minimum target speed");
	require_positive(assist.maximum_target_speed_mps, "maximum target speed");
	require_positive(assist.base_command_normalized, "auto-throttle base command");
	require_positive(assist.speed_kp, "auto-throttle proportional gain");
	require_positive(assist.speed_ki, "auto-throttle integral gain");
	require_positive(
		assist.speed_error_integral_limit_m,
		"auto-throttle speed-error integral limit");
	require_positive(
		assist.command_limit_normalized,
		"auto-throttle command limit");
	require_positive(assist.mach_guard_throttle_rate_per_s, "Mach guard rate");
	require_positive(
		config.pitch_tracking_error_limit_rad, "pitch tracking error limit");
	require_positive(
		config.vertical_speed_tracking_error_limit_mps,
		"vertical-speed tracking error limit");
	require_positive(
		config.bank_tracking_error_limit_rad, "bank tracking error limit");
	require_positive(
		config.tracking_failure_persistence_s,
		"tracking failure persistence");
	require_positive(
		config.actuator_saturation_persistence_s,
		"actuator saturation persistence");
}

void validate_threshold_ordering(
	const Core::Systems::AutomaticFlightControlConfig& config)
{
	const auto& assist = config.experimental_auto_throttle;
	if (config.altitude_fine_band_m >= config.altitude_hold_band_m ||
		config.altitude_hold_band_m >= config.altitude_capture_band_m ||
		assist.minimum_target_speed_mps >= assist.maximum_target_speed_mps ||
		config.capture_minimum_vertical_speed_mps +
			config.vertical_speed_capture_taper_mps >
			config.vertical_speed_limit_mps ||
		assist.maximum_mach > assist.disconnect_mach)
	{
		throw std::invalid_argument(
			"Automatic flight control thresholds are not ordered.");
	}
	if (config.pitch_stick_steering_threshold_normalized > 1.0 ||
		assist.base_command_normalized > 1.0 ||
		assist.command_limit_normalized > 1.0 ||
		config.capture_comfort_distance_factor > 1.0 ||
		config.approach_comfort_distance_factor > 1.0)
	{
		throw std::invalid_argument(
			"Automatic flight control normalized value is out of range.");
	}
}

Core::Systems::AutomaticFlightControlConfig project_defined_afcs_config()
{
	Core::Systems::AutomaticFlightControlConfig config;
	config.minimum_ias_mps = 240.0 * kMetersPerSecondPerKnot;
	config.engage_roll_limit_rad = Common::rad(45.0);
	config.engage_pitch_limit_rad = Common::rad(45.0);
	config.pitch_reference_rate_rad_s = Common::rad(10.0);
	config.vertical_reference_acceleration_mps2 = 1.0;
	config.altitude_fine_band_m = 50.0 * kMetersPerFoot;
	config.altitude_hold_band_m = 500.0 * kMetersPerFoot;
	config.altitude_capture_band_m = 1000.0 * kMetersPerFoot;
	config.altitude_comfort_acceleration_mps2 = 0.15 * kGravityMps2;
	config.altitude_fine_gain_s_inv = 0.04;
	config.altitude_hold_gain_s_inv = 0.08;
	config.altitude_capture_gain_s_inv = 0.12;
	config.altitude_approach_gain_s_inv = 0.08;
	config.capture_minimum_vertical_speed_mps = 1.0;
	config.capture_comfort_distance_factor = 0.8;
	config.approach_comfort_distance_factor = 0.6;
	config.vertical_speed_limit_mps = 15.0;
	config.vertical_speed_capture_taper_mps = 8.0;
	config.pitch_stick_steering_threshold_normalized = 0.08;
	config.altitude_step_m = 100.0 * kMetersPerFoot;
	config.heading_step_rad = Common::rad(1.0);
	config.pitch_step_rad = Common::rad(1.0);
	config.vertical_speed_step_mps = 1.0;
	config.pitch_tracking_error_limit_rad = Common::rad(8.0);
	config.vertical_speed_tracking_error_limit_mps = 12.0;
	config.bank_tracking_error_limit_rad = Common::rad(12.0);
	config.tracking_failure_persistence_s = 6.0;
	config.actuator_saturation_persistence_s = 1.0;
	config.heading_kp = 2.5;
	config.heading_ki = 0.1;
	// Project-defined pending F-CK-1C-specific control-law evidence.
	config.heading_error_integral_limit_rad_s = 0.5;
	return config;
}

Core::Systems::ExperimentalAutoThrottleAssistConfig
project_defined_auto_throttle_config()
{
	Core::Systems::ExperimentalAutoThrottleAssistConfig config;
	config.maximum_mach = 0.95;
	config.disconnect_mach = 1.0;
	config.speed_step_mps = 5.0 * kMetersPerSecondPerKnot;
	config.minimum_target_speed_mps = 200.0 * kMetersPerSecondPerKnot;
	config.maximum_target_speed_mps = 550.0 * kMetersPerSecondPerKnot;
	config.base_command_normalized = 0.5;
	config.speed_kp = 0.015;
	config.speed_ki = 0.003;
	// Project-defined experimental assist limit, not an aircraft claim.
	config.speed_error_integral_limit_m = 30.0;
	config.command_limit_normalized = 0.95;
	config.mach_guard_throttle_rate_per_s = 0.5;
	return config;
}
}

namespace Core
{
namespace Systems
{
AutomaticFlightControlConfig fck1c_automatic_flight_control_config(
	const ::Systems::FBWControllerConfig& control_laws)
{
	AutomaticFlightControlConfig config = project_defined_afcs_config();
	// F-16 Reference-derived envelope; the FCC validates source coherence.
	config.bank_limit_rad = control_laws.guidance_bank_limit_rad;
	config.roll_reference_rate_rad_s =
		control_laws.guidance_roll_rate_limit_rad_s;
	config.experimental_auto_throttle =
		project_defined_auto_throttle_config();
	return config;
}

AutomaticFlightControlConfig fck1c_automatic_flight_control_config()
{
	return fck1c_automatic_flight_control_config(
		::Systems::FBWControllerConfig());
}

void validate_automatic_flight_control_config(
	const AutomaticFlightControlConfig& config)
{
	validate_guidance_tuning(config);
	validate_reference_controls(config);
	validate_monitor_and_auto_throttle(config);
	validate_threshold_ordering(config);
}
}
}
