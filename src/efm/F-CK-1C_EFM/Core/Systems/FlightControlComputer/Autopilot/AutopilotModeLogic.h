#pragma once

#include "AutomaticFlightControlTypes.h"
#include "AutopilotModeMonitor.h"
#include "../ControlLaws/ConfigurationAndMode.h"
#include "../../../Contracts/CockpitContracts.h"
#include "../../../Contracts/Commands.h"

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
	bool vertical_degraded = false;
	bool lateral_degraded = false;
	AutomaticFlightControlVerticalMode vertical_mode =
		AutomaticFlightControlVerticalMode::Off;
	AutomaticFlightControlLateralMode lateral_mode =
		AutomaticFlightControlLateralMode::Off;
	double target_pitch_rad = 0.0;
	double target_vertical_speed_mps = 0.0;
	double target_altitude_m = 0.0;
	double target_heading_rad = 0.0;
	double target_heading_select_rad = 0.0;
	std::uint64_t vertical_reference_revision = 0;
	std::uint64_t lateral_reference_revision = 0;
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
	bool handle_vertical_command(const Command& command);
	bool handle_lateral_command(const Command& command);
	bool can_engage();
	void engage_autopilot();
	void disengage_all(
		AutomaticFlightControlReason reason,
		DisconnectReason disconnect_reason);
	void engage_pitch_hold();
	void engage_vertical_speed_hold();
	void engage_altitude_hold();
	void engage_heading_hold();
	void engage_heading_select();
	void adjust_vertical_reference(double direction);
	void adjust_lateral_reference(double direction);
	void set_bypass(bool requested);
	void recapture_vertical_reference();
	void recapture_lateral_reference();
	void update_pitch_stick_steering();
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
