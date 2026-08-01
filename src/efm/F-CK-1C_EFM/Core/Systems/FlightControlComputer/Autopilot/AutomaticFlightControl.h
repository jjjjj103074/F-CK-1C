#pragma once

#include "AutomaticFlightControlTypes.h"
#include "ExperimentalAutoThrottleAssist.h"
#include "LateralGuidance.h"
#include "VerticalGuidance.h"
#include "../../../Contracts/CockpitContracts.h"
#include "../../../Contracts/Commands.h"

#include <cstdint>
#include <vector>

namespace Core
{
namespace Systems
{
class SystemSetup;

class AutomaticFlightControl final
{
public:
	AutomaticFlightControl(
		const AutomaticFlightControlConfig& config,
		bool initial_weight_on_wheels);

	void register_commands(SystemSetup& setup);
	void handle_command(const Command& command);
	const AutomaticFlightGuidanceReference& step(
		const AutomaticFlightControlObservation& observation);
	const AutomaticFlightControlSnapshot& snapshot() const;

private:
	void apply_pending_commands();
	void apply_command(const Command& command);
	bool handle_master_command(const Command& command);
	bool handle_vertical_command(const Command& command);
	bool handle_lateral_command(const Command& command);
	bool can_engage_autopilot();
	void engage_autopilot();
	void disengage_all(AutomaticFlightControlReason reason);
	void engage_pitch_hold();
	void engage_vertical_speed_hold();
	void engage_altitude_hold();
	void engage_heading_hold();
	void engage_heading_select();
	void adjust_vertical_reference(double direction);
	void adjust_lateral_reference(double direction);
	void set_bypass(bool requested);
	void enter_bypass();
	void exit_bypass();
	void recapture_vertical_reference();
	void recapture_lateral_reference();
	void update_pitch_stick_steering();
	void apply_disconnect_guards();
	void reset_vertical_controller();
	void reset_lateral_controller();
	void refresh_reference();
	void refresh_snapshot();

	const AutomaticFlightControlConfig config_;
	VerticalGuidance vertical_guidance_;
	LateralGuidance lateral_guidance_;
	ExperimentalAutoThrottleAssist auto_throttle_;
	AutomaticFlightControlObservation observation_;
	AutomaticFlightGuidanceReference reference_;
	AutomaticFlightControlSnapshot snapshot_;
	std::vector<Command> pending_commands_;
	std::uint64_t revision_ = 0;
	bool master_engaged_ = false;
	bool bypass_active_ = false;
	bool pitch_stick_steering_active_ = false;
	bool heading_select_initialized_ = false;
	double target_pitch_rad_ = 0.0;
	double target_vertical_speed_mps_ = 0.0;
	double target_altitude_m_ = 0.0;
	double target_heading_rad_ = 0.0;
	double target_heading_select_rad_ = 0.0;
	VerticalGuidanceReference vertical_reference_;
	LateralGuidanceReference lateral_reference_;
	AutomaticFlightControlVerticalMode vertical_mode_ =
		AutomaticFlightControlVerticalMode::Off;
	AutomaticFlightControlLateralMode lateral_mode_ =
		AutomaticFlightControlLateralMode::Off;
	AutomaticFlightControlReason autopilot_rejection_reason_ =
		AutomaticFlightControlReason::None;
	AutomaticFlightControlReason autopilot_disengage_reason_ =
		AutomaticFlightControlReason::None;
};

AutomaticFlightControlConfig fck1c_automatic_flight_control_config();
void validate_automatic_flight_control_config(
	const AutomaticFlightControlConfig& config);
}
}
