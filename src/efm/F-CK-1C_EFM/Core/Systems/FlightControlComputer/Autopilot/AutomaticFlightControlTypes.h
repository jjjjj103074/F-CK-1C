#pragma once

namespace Core
{
namespace Systems
{
struct AutomaticFlightControlConfig
{
	double minimum_ias_mps = 0.0;
	double maximum_auto_throttle_mach = 0.0;
	double auto_throttle_disconnect_mach = 0.0;
	double engage_roll_limit_rad = 0.0;
	double engage_pitch_limit_rad = 0.0;
	double bank_limit_rad = 0.0;
	double pitch_command_limit = 0.0;
	double altitude_fine_band_m = 0.0;
	double altitude_hold_band_m = 0.0;
	double altitude_capture_band_m = 0.0;
	double altitude_comfort_acceleration_mps2 = 0.0;
	double vertical_speed_limit_mps = 0.0;
	double vertical_speed_capture_taper_mps = 0.0;
	double bypass_attitude_threshold_rad = 0.0;
	double speed_step_mps = 0.0;
	double minimum_target_speed_mps = 0.0;
	double maximum_target_speed_mps = 0.0;
	double altitude_step_m = 0.0;
	double heading_step_rad = 0.0;
	double pitch_step_rad = 0.0;
	double vertical_speed_step_mps = 0.0;
	double pitch_kp = 0.0;
	double pitch_kd = 0.0;
	double vertical_speed_kp = 0.0;
	double vertical_speed_ki = 0.0;
	double altitude_vertical_speed_kp = 0.0;
	double altitude_vertical_speed_ki = 0.0;
	double heading_kp = 0.0;
	double heading_ki = 0.0;
	double bank_kp = 0.0;
	double bank_kd = 0.0;
	double auto_throttle_base = 0.0;
	double auto_throttle_kp = 0.0;
	double auto_throttle_ki = 0.0;
	double auto_throttle_command_limit = 0.0;
	double mach_guard_throttle_rate_per_s = 0.0;
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
	double legacy_pitch_damping_rate_rad_s = 0.0;
	double legacy_heading_damping_rate_rad_s = 0.0;
	bool weight_on_wheels = false;
};

struct LegacyAutomaticFlightControlDemand
{
	bool pitch_roll_engaged = false;
	bool auto_throttle_engaged = false;
	double pitch_normalized = 0.0;
	double roll_normalized = 0.0;
	double throttle_normalized = 0.0;
};
}
}
