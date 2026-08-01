#include "AutomaticFlightControl.h"
#include "../FlightControlComputerConfig.h"

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
}

namespace Core
{
namespace Systems
{
AutomaticFlightControlConfig fck1c_automatic_flight_control_config()
{
	AutomaticFlightControlConfig config;
	config.minimum_ias_mps = 240.0 * kMetersPerSecondPerKnot;
	config.maximum_auto_throttle_mach = 0.95;
	config.auto_throttle_disconnect_mach = 1.0;
	config.engage_roll_limit_rad = Common::rad(45.0);
	config.engage_pitch_limit_rad = Common::rad(45.0);
	config.bank_limit_rad =
		fck1c_flight_control_computer_config()
			.control_laws.guidance_bank_limit_rad;
	config.roll_reference_rate_rad_s =
		fck1c_flight_control_computer_config()
			.control_laws.guidance_roll_rate_limit_rad_s;
	config.pitch_reference_rate_rad_s = Common::rad(10.0);
	config.vertical_reference_acceleration_mps2 = 2.0;
	config.altitude_fine_band_m = 50.0 * kMetersPerFoot;
	config.altitude_hold_band_m = 500.0 * kMetersPerFoot;
	config.altitude_capture_band_m = 1000.0 * kMetersPerFoot;
	config.altitude_comfort_acceleration_mps2 = 0.6 * kGravityMps2;
	config.vertical_speed_limit_mps = 40.0;
	config.vertical_speed_capture_taper_mps = 10.0;
	config.pitch_stick_steering_threshold_normalized = 0.08;
	config.speed_step_mps = 5.0 * kMetersPerSecondPerKnot;
	config.minimum_target_speed_mps = 200.0 * kMetersPerSecondPerKnot;
	config.maximum_target_speed_mps = 550.0 * kMetersPerSecondPerKnot;
	config.altitude_step_m = 100.0 * kMetersPerFoot;
	config.heading_step_rad = Common::rad(1.0);
	config.pitch_step_rad = Common::rad(1.0);
	config.vertical_speed_step_mps = 1.0;
	config.pitch_kp = 2.5;
	config.pitch_kd = 0.3;
	config.vertical_speed_kp = 0.08;
	config.vertical_speed_ki = 0.02;
	config.altitude_vertical_speed_kp = 0.04;
	config.altitude_vertical_speed_ki = 0.015;
	config.heading_kp = 2.5;
	config.heading_ki = 0.1;
	config.bank_kp = 1.8;
	config.bank_kd = 0.2;
	config.auto_throttle_base = 0.5;
	config.auto_throttle_kp = 0.015;
	config.auto_throttle_ki = 0.003;
	config.auto_throttle_command_limit = 0.95;
	config.mach_guard_throttle_rate_per_s = 0.5;
	return config;
}

void validate_automatic_flight_control_config(
	const AutomaticFlightControlConfig& config)
{
	require_positive(config.minimum_ias_mps, "minimum IAS");
	require_positive(config.maximum_auto_throttle_mach, "Mach guard");
	require_positive(config.auto_throttle_disconnect_mach, "Mach disconnect");
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
	require_positive(config.vertical_speed_limit_mps, "vertical speed limit");
	require_positive(
		config.pitch_stick_steering_threshold_normalized,
		"pitch stick-steering threshold");
	require_positive(config.speed_step_mps, "speed step");
	require_positive(config.altitude_step_m, "altitude step");
	require_positive(config.heading_step_rad, "heading step");
	require_positive(config.mach_guard_throttle_rate_per_s, "Mach guard rate");
	if (config.altitude_fine_band_m >= config.altitude_hold_band_m ||
		config.altitude_hold_band_m >= config.altitude_capture_band_m ||
		config.maximum_auto_throttle_mach >
			config.auto_throttle_disconnect_mach)
	{
		throw std::invalid_argument(
			"Automatic flight control thresholds are not ordered.");
	}
	if (config.pitch_stick_steering_threshold_normalized > 1.0)
	{
		throw std::invalid_argument(
			"Automatic flight control stick-steering threshold is out of range.");
	}
}
}
}
