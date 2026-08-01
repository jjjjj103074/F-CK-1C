#include "AutopilotModeLogic.h"

#include "Common/Angles.h"
#include "Common/Units.h"

#include <cmath>

namespace
{
constexpr double kEnabledCommandThreshold = 0.5;
constexpr int kLastAutopilotModeCommandOffset = 13;

// Commands.h intentionally keeps the AP mode command block contiguous.
static_assert(
	static_cast<int>(Core::CommandId::ToggleAutopilotMaster) +
		kLastAutopilotModeCommandOffset ==
	static_cast<int>(Core::CommandId::DecreaseAutopilotLateralReference),
	"AutopilotModeLogic::handles requires a contiguous command block.");

bool pressed(const Core::Command& command)
{
	return command.value > kEnabledCommandThreshold;
}

bool is_vertical_command(Core::CommandId id)
{
	switch (id)
	{
	case Core::CommandId::SelectAutopilotPitchHold:
	case Core::CommandId::SelectAutopilotVerticalSpeedHold:
	case Core::CommandId::SelectAutopilotAltitudeHold:
	case Core::CommandId::IncreaseAutopilotVerticalReference:
	case Core::CommandId::DecreaseAutopilotVerticalReference:
		return true;
	default:
		return false;
	}
}

bool is_lateral_command(Core::CommandId id)
{
	switch (id)
	{
	case Core::CommandId::SelectAutopilotHeadingHold:
	case Core::CommandId::SelectAutopilotHeading:
	case Core::CommandId::SelectAutopilotNavigationTrack:
	case Core::CommandId::IncreaseAutopilotLateralReference:
	case Core::CommandId::DecreaseAutopilotLateralReference:
		return true;
	default:
		return false;
	}
}
}

namespace Core
{
namespace Systems
{
AutopilotModeLogic::AutopilotModeLogic(
	const AutomaticFlightControlConfig& config)
	: config_(config)
{
}

bool AutopilotModeLogic::handles(CommandId id)
{
	return id >= CommandId::ToggleAutopilotMaster &&
		id <= CommandId::DecreaseAutopilotLateralReference;
}

void AutopilotModeLogic::handle_command(const Command& command)
{
	pending_commands_.push_back(command);
}

void AutopilotModeLogic::update(
	const AutomaticFlightControlObservation& observation,
	const AutopilotModeMonitorResult& monitor_result,
	const ::Systems::ManeuverEnvelope& envelope)
{
	observation_ = observation;
	envelope_ = envelope;
	apply_monitor_result(monitor_result);
	apply_pending_commands();
	apply_disconnect_guards();
	update_pitch_stick_steering();
}

void AutopilotModeLogic::apply_pending_commands()
{
	for (const Command& command : pending_commands_)
	{
		apply_command(command);
	}
	pending_commands_.clear();
}

void AutopilotModeLogic::apply_command(const Command& command)
{
	if (handle_master_command(command)) return;
	if (handle_vertical_command(command)) return;
	(void)handle_lateral_command(command);
}

bool AutopilotModeLogic::handle_master_command(const Command& command)
{
	switch (command.id)
	{
	case CommandId::ToggleAutopilotMaster:
		if (!pressed(command)) return true;
		if (state_.master_engaged)
			disengage_all(
				AutomaticFlightControlReason::Commanded,
				DisconnectReason::PilotCommand);
		else engage_autopilot();
		return true;
	case CommandId::EngageAutopilot:
		if (pressed(command) && !state_.master_engaged) engage_autopilot();
		return true;
	case CommandId::DisengageAutopilot:
		if (pressed(command)) disengage_all(
			AutomaticFlightControlReason::Commanded,
			DisconnectReason::PilotCommand);
		return true;
	case CommandId::SetAutopilotBypass:
		set_bypass(pressed(command));
		return true;
	default:
		return false;
	}
}

bool AutopilotModeLogic::handle_vertical_command(const Command& command)
{
	if (!is_vertical_command(command.id)) return false;
	if (!pressed(command) || !state_.master_engaged) return true;
	switch (command.id)
	{
	case CommandId::SelectAutopilotPitchHold:
		engage_pitch_hold();
		return true;
	case CommandId::SelectAutopilotVerticalSpeedHold:
		engage_vertical_speed_hold();
		return true;
	case CommandId::SelectAutopilotAltitudeHold:
		engage_altitude_hold();
		return true;
	case CommandId::IncreaseAutopilotVerticalReference:
		adjust_vertical_reference(1.0);
		return true;
	case CommandId::DecreaseAutopilotVerticalReference:
		adjust_vertical_reference(-1.0);
		return true;
	default:
		return false;
	}
}

bool AutopilotModeLogic::handle_lateral_command(const Command& command)
{
	if (!is_lateral_command(command.id)) return false;
	if (!pressed(command) || !state_.master_engaged) return true;
	switch (command.id)
	{
	case CommandId::SelectAutopilotHeadingHold:
		engage_heading_hold();
		return true;
	case CommandId::SelectAutopilotHeading:
		engage_heading_select();
		return true;
	case CommandId::SelectAutopilotNavigationTrack:
		state_.lateral_mode =
			AutomaticFlightControlLateralMode::NavigationTrack;
		++state_.lateral_reference_revision;
		return true;
	case CommandId::IncreaseAutopilotLateralReference:
		adjust_lateral_reference(1.0);
		return true;
	case CommandId::DecreaseAutopilotLateralReference:
		adjust_lateral_reference(-1.0);
		return true;
	default:
		return false;
	}
}

bool AutopilotModeLogic::can_engage()
{
	state_.engage_rejection_reason = AutomaticFlightControlReason::None;
	if (observation_.indicated_airspeed_mps < config_.minimum_ias_mps)
		state_.engage_rejection_reason =
			AutomaticFlightControlReason::BelowMinimumIndicatedAirspeed;
	else if (observation_.weight_on_wheels)
		state_.engage_rejection_reason = AutomaticFlightControlReason::WeightOnWheels;
	else if (std::abs(observation_.roll_rad) > config_.engage_roll_limit_rad)
		state_.engage_rejection_reason = AutomaticFlightControlReason::RollLimit;
	else if (std::abs(observation_.pitch_rad) > config_.engage_pitch_limit_rad)
		state_.engage_rejection_reason = AutomaticFlightControlReason::PitchLimit;
	return state_.engage_rejection_reason == AutomaticFlightControlReason::None;
}

void AutopilotModeLogic::engage_autopilot()
{
	if (!can_engage()) return;
	state_.master_engaged = true;
	state_.disengage_reason = AutomaticFlightControlReason::None;
	state_.disconnect_reason = DisconnectReason::None;
	state_.degradation_reason = DegradationReason::None;
	state_.vertical_degraded = false;
	state_.lateral_degraded = false;
	engage_pitch_hold();
	engage_heading_hold();
}

void AutopilotModeLogic::disengage_all(
	AutomaticFlightControlReason reason,
	DisconnectReason disconnect_reason)
{
	state_.master_engaged = false;
	state_.disengage_reason = reason;
	state_.disconnect_reason = disconnect_reason;
	release_vertical();
	release_lateral();
	state_.bypass_active = false;
	state_.pitch_stick_steering_active = false;
}

void AutopilotModeLogic::engage_pitch_hold()
{
	state_.vertical_mode = AutomaticFlightControlVerticalMode::PitchHold;
	state_.target_pitch_rad = observation_.pitch_rad;
	++state_.vertical_reference_revision;
}

void AutopilotModeLogic::engage_vertical_speed_hold()
{
	state_.vertical_mode =
		AutomaticFlightControlVerticalMode::VerticalSpeedHold;
	state_.target_vertical_speed_mps = observation_.vertical_speed_mps;
	++state_.vertical_reference_revision;
}

void AutopilotModeLogic::engage_altitude_hold()
{
	state_.vertical_mode = AutomaticFlightControlVerticalMode::AltitudeHold;
	state_.target_altitude_m = observation_.altitude_m;
	++state_.vertical_reference_revision;
}

void AutopilotModeLogic::engage_heading_hold()
{
	state_.lateral_mode = AutomaticFlightControlLateralMode::HeadingHold;
	state_.target_heading_rad =
		Common::wrap_heading_rad(observation_.heading_rad);
	++state_.lateral_reference_revision;
}

void AutopilotModeLogic::engage_heading_select()
{
	state_.lateral_mode = AutomaticFlightControlLateralMode::HeadingSelect;
	if (!heading_select_initialized_)
	{
		state_.target_heading_select_rad =
			Common::wrap_heading_rad(observation_.heading_rad);
		heading_select_initialized_ = true;
	}
	++state_.lateral_reference_revision;
}

void AutopilotModeLogic::adjust_vertical_reference(double direction)
{
	switch (state_.vertical_mode)
	{
	case AutomaticFlightControlVerticalMode::AltitudeHold:
		state_.target_altitude_m += direction * config_.altitude_step_m;
		++state_.vertical_reference_revision;
		break;
	case AutomaticFlightControlVerticalMode::VerticalSpeedHold:
		state_.target_vertical_speed_mps +=
			direction * config_.vertical_speed_step_mps;
		++state_.vertical_reference_revision;
		break;
	case AutomaticFlightControlVerticalMode::PitchHold:
		state_.target_pitch_rad += direction * config_.pitch_step_rad;
		++state_.vertical_reference_revision;
		break;
	default:
		break;
	}
}

void AutopilotModeLogic::adjust_lateral_reference(double direction)
{
	if (state_.lateral_mode == AutomaticFlightControlLateralMode::HeadingHold)
	{
		state_.target_heading_rad = Common::wrap_heading_rad(
			state_.target_heading_rad + direction * config_.heading_step_rad);
		++state_.lateral_reference_revision;
		return;
	}
	if (state_.lateral_mode == AutomaticFlightControlLateralMode::HeadingSelect)
	{
		state_.target_heading_select_rad = Common::wrap_heading_rad(
			state_.target_heading_select_rad +
				direction * config_.heading_step_rad);
		++state_.lateral_reference_revision;
	}
}

void AutopilotModeLogic::set_bypass(bool requested)
{
	if (requested)
	{
		if (state_.master_engaged) state_.bypass_active = true;
		return;
	}
	if (!state_.bypass_active) return;
	state_.bypass_active = false;
	recapture_vertical_reference();
	recapture_lateral_reference();
}

void AutopilotModeLogic::recapture_vertical_reference()
{
	switch (state_.vertical_mode)
	{
	case AutomaticFlightControlVerticalMode::PitchHold:
		engage_pitch_hold(); break;
	case AutomaticFlightControlVerticalMode::VerticalSpeedHold:
		engage_vertical_speed_hold(); break;
	case AutomaticFlightControlVerticalMode::AltitudeHold:
		engage_altitude_hold(); break;
	default:
		break;
	}
}

void AutopilotModeLogic::recapture_lateral_reference()
{
	if (state_.lateral_mode == AutomaticFlightControlLateralMode::HeadingHold)
	{
		engage_heading_hold();
	}
}

void AutopilotModeLogic::update_pitch_stick_steering()
{
	const bool supported = state_.master_engaged &&
		state_.vertical_mode == AutomaticFlightControlVerticalMode::PitchHold &&
		!state_.bypass_active;
	const bool requested = supported &&
		std::abs(observation_.conditioned_pitch_input_normalized) >
			config_.pitch_stick_steering_threshold_normalized;
	if (requested)
	{
		state_.pitch_stick_steering_active = true;
		state_.target_pitch_rad = observation_.pitch_rad;
		++state_.vertical_reference_revision;
		return;
	}
	if (state_.pitch_stick_steering_active)
	{
		state_.target_pitch_rad = observation_.pitch_rad;
		++state_.vertical_reference_revision;
	}
	state_.pitch_stick_steering_active = false;
}

void AutopilotModeLogic::apply_disconnect_guards()
{
	if (!state_.master_engaged) return;
	if (observation_.indicated_airspeed_mps < config_.minimum_ias_mps)
	{
		disengage_all(
			AutomaticFlightControlReason::BelowMinimumIndicatedAirspeed,
			DisconnectReason::SafetyCondition);
		return;
	}
	if (observation_.weight_on_wheels)
	{
		disengage_all(
			AutomaticFlightControlReason::WeightOnWheels,
			DisconnectReason::SafetyCondition);
		return;
	}
	if (std::abs(observation_.roll_rad) >
		envelope_.hard_protection.bank_limit_rad)
	{
		disengage_all(
			AutomaticFlightControlReason::RollLimit,
			DisconnectReason::SafetyCondition);
	}
}

void AutopilotModeLogic::apply_monitor_result(
	const AutopilotModeMonitorResult& result)
{
	if (result.disconnect_reason != DisconnectReason::None)
	{
		disengage_all(
			AutomaticFlightControlReason::Commanded,
			result.disconnect_reason);
		return;
	}
	if (result.release_vertical)
	{
		release_vertical();
		state_.vertical_degraded = true;
	}
	if (result.release_lateral)
	{
		release_lateral();
		state_.lateral_degraded = true;
	}
	if (result.degradation_reason != DegradationReason::None)
	{
		state_.degradation_reason = result.degradation_reason;
	}
}

void AutopilotModeLogic::release_vertical()
{
	state_.vertical_mode = AutomaticFlightControlVerticalMode::Off;
	state_.pitch_stick_steering_active = false;
	++state_.vertical_reference_revision;
}

void AutopilotModeLogic::release_lateral()
{
	state_.lateral_mode = AutomaticFlightControlLateralMode::Off;
	++state_.lateral_reference_revision;
}

const AutopilotModeLogicState& AutopilotModeLogic::state() const
{
	return state_;
}
}
}
