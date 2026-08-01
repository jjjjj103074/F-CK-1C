#pragma once

#include "ExperimentalAutoThrottleAssistConfig.h"
#include "../FlightControlReferences.h"

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
	double vertical_reference_acceleration_mps2 = 0.0;
	double altitude_fine_band_m = 0.0;
	double altitude_hold_band_m = 0.0;
	double altitude_capture_band_m = 0.0;
	double altitude_comfort_acceleration_mps2 = 0.0;
	double altitude_fine_gain_s_inv = 0.0;
	double altitude_hold_gain_s_inv = 0.0;
	double altitude_capture_gain_s_inv = 0.0;
	double altitude_approach_gain_s_inv = 0.0;
	double capture_minimum_vertical_speed_mps = 0.0;
	double capture_comfort_distance_factor = 0.0;
	double approach_comfort_distance_factor = 0.0;
	double vertical_speed_limit_mps = 0.0;
	double vertical_speed_capture_taper_mps = 0.0;
	double pitch_stick_steering_threshold_normalized = 0.0;
	double altitude_step_m = 0.0;
	double heading_step_rad = 0.0;
	double pitch_step_rad = 0.0;
	double vertical_speed_step_mps = 0.0;
	double pitch_tracking_error_limit_rad = 0.0;
	double vertical_speed_tracking_error_limit_mps = 0.0;
	double bank_tracking_error_limit_rad = 0.0;
	double tracking_failure_persistence_s = 0.0;
	double actuator_saturation_persistence_s = 0.0;
	double heading_kp = 0.0;
	double heading_ki = 0.0;
	double heading_error_integral_limit_rad_s = 0.0;
	ExperimentalAutoThrottleAssistConfig experimental_auto_throttle;
};

struct AutomaticFlightControlObservation
{
	double dt_s = 0.0;
	double indicated_airspeed_mps = 0.0;
	double altitude_m = 0.0;
	double vertical_speed_mps = 0.0;
	double mach = 0.0;
	double heading_rad = 0.0;
	double pitch_rad = 0.0;
	double roll_rad = 0.0;
	double conditioned_pitch_input_normalized = 0.0;
	double conditioned_roll_input_normalized = 0.0;
	bool weight_on_wheels = false;
};

}
}
