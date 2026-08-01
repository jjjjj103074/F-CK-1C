#pragma once

#include "../../../Contracts/CockpitContracts.h"
#include "../../../Contracts/Commands.h"

#include <cstdint>
#include <vector>

namespace Core
{
namespace Systems
{
class SystemSetup;

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

class AutomaticFlightControl final
{
public:
	AutomaticFlightControl(
		const AutomaticFlightControlConfig& config,
		bool initial_weight_on_wheels);

	void register_commands(SystemSetup& setup);
	void handle_command(const Command& command);
	const LegacyAutomaticFlightControlDemand& step(
		const AutomaticFlightControlObservation& observation);
	const AutomaticFlightControlSnapshot& snapshot() const;

private:
	void apply_pending_commands();
	void apply_command(const Command& command);
	bool handle_master_command(const Command& command);
	bool handle_vertical_command(const Command& command);
	bool handle_lateral_command(const Command& command);
	bool handle_auto_throttle_command(const Command& command);
	bool can_engage_autopilot();
	bool can_engage_auto_throttle();
	void engage_autopilot();
	void disengage_all(AutomaticFlightControlReason reason);
	void engage_pitch_hold();
	void engage_vertical_speed_hold();
	void engage_altitude_hold();
	void engage_heading_hold();
	void engage_heading_select();
	void engage_auto_throttle();
	void disengage_auto_throttle(AutomaticFlightControlReason reason);
	void adjust_vertical_reference(double direction);
	void adjust_lateral_reference(double direction);
	void adjust_speed_reference(double direction);
	void set_bypass(bool requested);
	void enter_bypass();
	void exit_bypass();
	bool bypass_change_is_meaningful() const;
	void recapture_vertical_reference();
	void recapture_lateral_reference();
	void apply_disconnect_guards();
	void update_vertical_controller();
	void update_lateral_controller();
	void update_pitch_hold();
	void update_vertical_speed_hold();
	void update_altitude_hold();
	void update_heading_control(double target_heading_rad);
	void update_auto_throttle();
	double altitude_desired_vertical_speed(double altitude_error_m) const;
	double comfort_limited_vertical_speed(
		double altitude_error_m,
		double maximum_vertical_speed_mps,
		double remaining_distance_factor) const;
	void reset_vertical_controller();
	void reset_lateral_controller();
	void reset_auto_throttle_controller();
	void refresh_demand();
	void refresh_snapshot();

	const AutomaticFlightControlConfig config_;
	AutomaticFlightControlObservation observation_;
	LegacyAutomaticFlightControlDemand demand_;
	AutomaticFlightControlSnapshot snapshot_;
	std::vector<Command> pending_commands_;
	std::uint64_t revision_ = 0;
	bool master_engaged_ = false;
	bool auto_throttle_engaged_ = false;
	bool bypass_active_ = false;
	bool heading_select_initialized_ = false;
	double target_pitch_rad_ = 0.0;
	double target_vertical_speed_mps_ = 0.0;
	double target_altitude_m_ = 0.0;
	double target_heading_rad_ = 0.0;
	double target_heading_select_rad_ = 0.0;
	double target_speed_mps_ = 0.0;
	double pitch_command_ = 0.0;
	double roll_command_ = 0.0;
	double throttle_command_ = 0.0;
	double bypass_start_pitch_rad_ = 0.0;
	double bypass_start_roll_rad_ = 0.0;
	double bypass_start_heading_rad_ = 0.0;
	double vertical_speed_integral_ = 0.0;
	double heading_integral_ = 0.0;
	double speed_integral_ = 0.0;
	AutomaticFlightControlVerticalMode vertical_mode_ =
		AutomaticFlightControlVerticalMode::Off;
	AutomaticFlightControlLateralMode lateral_mode_ =
		AutomaticFlightControlLateralMode::Off;
	AutomaticFlightControlReason autopilot_rejection_reason_ =
		AutomaticFlightControlReason::None;
	AutomaticFlightControlReason autopilot_disengage_reason_ =
		AutomaticFlightControlReason::None;
	AutomaticFlightControlReason auto_throttle_rejection_reason_ =
		AutomaticFlightControlReason::None;
	AutomaticFlightControlReason auto_throttle_disengage_reason_ =
		AutomaticFlightControlReason::None;
};

AutomaticFlightControlConfig fck1c_automatic_flight_control_config();
void validate_automatic_flight_control_config(
	const AutomaticFlightControlConfig& config);
}
}
