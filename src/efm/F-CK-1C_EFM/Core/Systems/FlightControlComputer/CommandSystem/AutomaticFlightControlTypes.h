#pragma once

#include "ExperimentalAutoThrottleAssistConfig.h"
#include "../Contracts/FlightControlReferences.h"

// Public configuration contract; AFCS behavior remains CommandSystem internal.

namespace Systems
{
struct ModeAndGainSchedulingConfig;
}

namespace Core
{
namespace Systems
{
struct AutomaticFlightControlConfig
{
	double minimum_ias_mps = 0.0;
	double engage_roll_limit_rad = 0.0;
	double engage_pitch_limit_rad = 0.0;
	double bank_limit_rad = 0.0;
	double roll_reference_rate_rad_s = 0.0;
	double pitch_reference_rate_rad_s = 0.0;
	double vertical_reference_acceleration_ft_s2 = 0.0;
	double altitude_fine_band_ft = 0.0;
	double altitude_hold_band_ft = 0.0;
	double altitude_capture_band_ft = 0.0;
	double altitude_comfort_acceleration_ft_s2 = 0.0;
	double altitude_fine_gain_s_inv = 0.0;
	double altitude_hold_gain_s_inv = 0.0;
	double altitude_capture_gain_s_inv = 0.0;
	double altitude_approach_gain_s_inv = 0.0;
	double capture_minimum_vertical_speed_ft_s = 0.0;
	double capture_comfort_distance_factor = 0.0;
	double approach_comfort_distance_factor = 0.0;
	double vertical_speed_limit_ft_s = 0.0;
	double vertical_speed_capture_taper_ft_s = 0.0;
	double pitch_stick_steering_threshold_normalized = 0.0;
	double roll_stick_steering_threshold_normalized = 0.0;
	int heading_select_step_deg = 0;
	double pitch_tracking_error_limit_rad = 0.0;
	double vertical_speed_tracking_error_limit_ft_s = 0.0;
	double bank_tracking_error_limit_rad = 0.0;
	double tracking_failure_persistence_s = 0.0;
	double actuator_saturation_persistence_s = 0.0;
	double heading_kp = 0.0;
	double heading_ki = 0.0;
	double heading_error_integral_limit_deg_s = 0.0;
	ExperimentalAutoThrottleAssistConfig experimental_auto_throttle;
};

AutomaticFlightControlConfig fck1c_automatic_flight_control_config();
AutomaticFlightControlConfig fck1c_automatic_flight_control_config(
	const ::Systems::ModeAndGainSchedulingConfig& mode_and_gain);
void validate_automatic_flight_control_config(
	const AutomaticFlightControlConfig& config);
}
}
