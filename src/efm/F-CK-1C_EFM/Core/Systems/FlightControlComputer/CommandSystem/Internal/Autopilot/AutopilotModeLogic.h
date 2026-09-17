#pragma once

#include "../../AutomaticFlightControlTypes.h"
#include "AutomaticFlightControlObservation.h"
#include "AutopilotModeMonitor.h"
#include "../../../ModeAndGainScheduling/ModeAndGainScheduling.h"
#include "../../../../../Contracts/Commands.h"

#include <cstdint>
#include <vector>

namespace Core
{
namespace Systems
{
struct AutopilotModeLogicState
{
	bool master_engaged = false;
	bool bypass_active = false;
	bool pitch_stick_steering_active = false;
	bool roll_stick_steering_active = false;
	bool vertical_degraded = false;
	bool lateral_degraded = false;
	AutomaticFlightControlVerticalMode vertical_mode =
		AutomaticFlightControlVerticalMode::Off;
	AutomaticFlightControlLateralMode lateral_mode =
		AutomaticFlightControlLateralMode::Off;
	// F-16A/B-reference physical mode selectors keep their positions while the
	// separate AUTOPILOT master switch is OFF.
	AutomaticFlightControlVerticalMode selected_vertical_mode =
		AutomaticFlightControlVerticalMode::PitchAttitudeHold;
	AutomaticFlightControlLateralMode selected_lateral_mode =
		AutomaticFlightControlLateralMode::RollAttitudeHold;
	double target_pitch_rad = 0.0;
	double target_altitude_ft = 0.0;
	double target_roll_rad = 0.0;
	// Pilot-facing HSI selection remains an exact whole magnetic degree through
	// the heading loop. LateralGuidance emits the bank reference in radians.
	int target_heading_deg = 0;
	// Revisions request guidance state reinitialization. Ordinary selected-target
	// changes remain continuous and therefore do not advance these counters.
	std::uint64_t vertical_guidance_reset_revision = 0;
	std::uint64_t lateral_guidance_reset_revision = 0;
	AutomaticFlightControlReason engage_rejection_reason =
		AutomaticFlightControlReason::None;
	AutomaticFlightControlReason disengage_reason =
		AutomaticFlightControlReason::None;
	DegradationReason degradation_reason = DegradationReason::None;
	DisconnectReason disconnect_reason = DisconnectReason::None;
};

class AutopilotModeLogic final
{
public:
	explicit AutopilotModeLogic(const AutomaticFlightControlConfig& config);
	static bool handles(CommandId id);
	void handle_command(const Command& command);
	void update(
		const AutomaticFlightControlObservation& observation,
		const AutopilotModeMonitorResult& monitor_result,
		const ::Systems::ManeuverEnvelope& envelope);
	const AutopilotModeLogicState& state() const;

private:
	void apply_pending_commands();
	void apply_command(const Command& command);
	bool handle_master_command(const Command& command);
	bool handle_pitch_command(const Command& command);
	bool handle_lateral_command(const Command& command);
	void select_roll_attitude_mode();
	void select_heading_mode();
	bool can_engage();
	void engage_autopilot();
	void select_pitch_mode(AutomaticFlightControlVerticalMode mode);
	void begin_engagement();
	void disengage_all(
		AutomaticFlightControlReason reason,
		DisconnectReason disconnect_reason);
	void engage_pitch_attitude_hold();
	void engage_altitude_hold();
	void engage_roll_attitude_hold();
	void engage_heading_select();
	void capture_roll_reference();
	void clear_vertical_degradation();
	void clear_lateral_degradation();
	void adjust_heading_select(int direction);
	void initialize_heading_select();
	void set_bypass(bool requested);
	void recapture_vertical_reference();
	void recapture_lateral_reference();
	void update_pitch_stick_steering();
	void update_roll_stick_steering();
	void apply_disconnect_guards();
	void apply_monitor_result(const AutopilotModeMonitorResult& result);
	void release_vertical();
	void release_lateral();

	const AutomaticFlightControlConfig config_;
	AutomaticFlightControlObservation observation_;
	::Systems::ManeuverEnvelope envelope_;
	AutopilotModeLogicState state_;
	std::vector<Command> pending_commands_;
	bool heading_select_initialized_ = false;
};
}
}
