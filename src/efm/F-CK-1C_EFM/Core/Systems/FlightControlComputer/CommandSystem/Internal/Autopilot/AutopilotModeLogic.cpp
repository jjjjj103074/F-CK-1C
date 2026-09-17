#include "AutopilotModeLogic.h"

// Private AFCS mode implementation; use FlightControlCommandSystem.

#include "Common/Angles.h"
#include "Common/Clamp.h"
#include "Common/Units.h"

#include <cmath>

namespace
{
constexpr double kEnabledCommandThreshold = 0.5;
constexpr int kLastAutopilotModeCommandOffset = 8;

// Commands.h intentionally keeps the AP mode command block contiguous.
static_assert(
	static_cast<int>(Core::CommandId::EngageAutopilot) +
		kLastAutopilotModeCommandOffset ==
	static_cast<int>(Core::CommandId::DecreaseAutopilotHeadingSelect),
	"AutopilotModeLogic::handles requires a contiguous command block.");

bool pressed(const Core::Command& command)
{
	return command.value_normalized > kEnabledCommandThreshold;
}

bool is_pitch_command(Core::CommandId id)
{
	switch (id)
	{
	case Core::CommandId::SetAutopilotBypass:
	case Core::CommandId::SelectAutopilotPitchAttitudeHold:
	case Core::CommandId::SelectAutopilotAltitudeHold:
		return true;
	default:
		return false;
	}
}

bool is_master_command(Core::CommandId id)
{
	return id == Core::CommandId::EngageAutopilot ||
		id == Core::CommandId::DisengageAutopilot;
}

bool is_lateral_command(Core::CommandId id)
{
	switch (id)
	{
	case Core::CommandId::SelectAutopilotRollAttitudeHold:
	case Core::CommandId::SelectAutopilotHeadingSelect:
	case Core::CommandId::IncreaseAutopilotHeadingSelect:
	case Core::CommandId::DecreaseAutopilotHeadingSelect:
		return true;
	default:
		return false;
	}
}

bool is_vertical_degradation(Core::Systems::DegradationReason reason)
{
	return reason ==
			Core::Systems::DegradationReason::SustainedVerticalTrackingFailure ||
		reason == Core::Systems::DegradationReason::PressureAltitudeUnavailable;
}

bool is_lateral_degradation(Core::Systems::DegradationReason reason)
{
	return reason ==
			Core::Systems::DegradationReason::SustainedLateralTrackingFailure ||
		reason == Core::Systems::DegradationReason::MagneticHeadingUnavailable;
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
	return id >= CommandId::EngageAutopilot &&
		id <= CommandId::DecreaseAutopilotHeadingSelect;
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
	update_roll_stick_steering();
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
	if (handle_pitch_command(command)) return;
	(void)handle_lateral_command(command);
}

bool AutopilotModeLogic::handle_master_command(const Command& command)
{
	if (!is_master_command(command.id) || !pressed(command)) return false;
	switch (command.id)
	{
	case CommandId::EngageAutopilot:
		engage_autopilot();
		return true;
	case CommandId::DisengageAutopilot:
		disengage_all(
			AutomaticFlightControlReason::Commanded,
			DisconnectReason::PilotCommand);
		return true;
	default:
		return false;
	}
}

bool AutopilotModeLogic::handle_pitch_command(const Command& command)
{
	if (!is_pitch_command(command.id)) return false;
	switch (command.id)
	{
	case CommandId::SetAutopilotBypass:
		set_bypass(pressed(command));
		return true;
	case CommandId::SelectAutopilotPitchAttitudeHold:
		if (pressed(command)) select_pitch_mode(
			AutomaticFlightControlVerticalMode::PitchAttitudeHold);
		return true;
	case CommandId::SelectAutopilotAltitudeHold:
		if (pressed(command)) select_pitch_mode(
			AutomaticFlightControlVerticalMode::AltitudeHold);
		return true;
	default:
		return false;
	}
}

bool AutopilotModeLogic::handle_lateral_command(const Command& command)
{
	if (!is_lateral_command(command.id)) return false;
	if (!pressed(command)) return true;
	switch (command.id)
	{
	case CommandId::SelectAutopilotRollAttitudeHold:
		select_roll_attitude_mode();
		return true;
	case CommandId::SelectAutopilotHeadingSelect:
		select_heading_mode();
		return true;
	case CommandId::IncreaseAutopilotHeadingSelect:
		adjust_heading_select(1);
		return true;
	case CommandId::DecreaseAutopilotHeadingSelect:
		adjust_heading_select(-1);
		return true;
	default:
		return false;
	}
}

void AutopilotModeLogic::select_roll_attitude_mode()
{
	const bool already_selected = state_.selected_lateral_mode ==
		AutomaticFlightControlLateralMode::RollAttitudeHold;
	state_.selected_lateral_mode =
		AutomaticFlightControlLateralMode::RollAttitudeHold;
	if (!state_.master_engaged) return;
	if (!already_selected || state_.lateral_mode !=
		AutomaticFlightControlLateralMode::RollAttitudeHold)
	{
		engage_roll_attitude_hold();
	}
}

void AutopilotModeLogic::select_heading_mode()
{
	const bool already_selected = state_.selected_lateral_mode ==
		AutomaticFlightControlLateralMode::HeadingSelect;
	state_.selected_lateral_mode =
		AutomaticFlightControlLateralMode::HeadingSelect;
	if (!already_selected) initialize_heading_select();
	if (!state_.master_engaged) return;
	if (!already_selected || state_.lateral_mode !=
		AutomaticFlightControlLateralMode::HeadingSelect)
	{
		engage_heading_select();
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
	else if (state_.selected_vertical_mode ==
		AutomaticFlightControlVerticalMode::AltitudeHold &&
		!observation_.pressure_altitude_available)
		state_.engage_rejection_reason =
			AutomaticFlightControlReason::PressureAltitudeUnavailable;
	else if (state_.selected_lateral_mode ==
		AutomaticFlightControlLateralMode::HeadingSelect &&
		!observation_.magnetic_heading_available)
		state_.engage_rejection_reason =
			AutomaticFlightControlReason::MagneticHeadingUnavailable;
	return state_.engage_rejection_reason == AutomaticFlightControlReason::None;
}

void AutopilotModeLogic::engage_autopilot()
{
	if (state_.master_engaged || !can_engage()) return;
	begin_engagement();
}

void AutopilotModeLogic::select_pitch_mode(
	AutomaticFlightControlVerticalMode mode)
{
	if (state_.selected_vertical_mode == mode) return;
	state_.selected_vertical_mode = mode;
	if (!state_.master_engaged) return;
	if (mode == AutomaticFlightControlVerticalMode::PitchAttitudeHold)
		engage_pitch_attitude_hold();
	else if (mode == AutomaticFlightControlVerticalMode::AltitudeHold)
		engage_altitude_hold();
}

void AutopilotModeLogic::begin_engagement()
{
	state_.master_engaged = true;
	state_.disengage_reason = AutomaticFlightControlReason::None;
	state_.disconnect_reason = DisconnectReason::None;
	state_.degradation_reason = DegradationReason::None;
	state_.vertical_degraded = false;
	state_.lateral_degraded = false;
	if (state_.selected_vertical_mode ==
		AutomaticFlightControlVerticalMode::PitchAttitudeHold)
		engage_pitch_attitude_hold();
	else engage_altitude_hold();
	if (state_.selected_lateral_mode ==
		AutomaticFlightControlLateralMode::RollAttitudeHold)
		engage_roll_attitude_hold();
	else if (state_.selected_lateral_mode ==
		AutomaticFlightControlLateralMode::HeadingSelect)
		engage_heading_select();
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
	state_.roll_stick_steering_active = false;
}

void AutopilotModeLogic::engage_pitch_attitude_hold()
{
	state_.vertical_mode =
		AutomaticFlightControlVerticalMode::PitchAttitudeHold;
	state_.target_pitch_rad = observation_.pitch_rad;
	clear_vertical_degradation();
	++state_.vertical_guidance_reset_revision;
}

void AutopilotModeLogic::engage_altitude_hold()
{
	if (!observation_.pressure_altitude_available)
	{
		release_vertical();
		state_.vertical_degraded = true;
		state_.degradation_reason =
			DegradationReason::PressureAltitudeUnavailable;
		return;
	}
	state_.vertical_mode = AutomaticFlightControlVerticalMode::AltitudeHold;
	state_.target_altitude_ft = observation_.pressure_altitude_ft;
	clear_vertical_degradation();
	++state_.vertical_guidance_reset_revision;
}

void AutopilotModeLogic::engage_roll_attitude_hold()
{
	state_.lateral_mode =
		AutomaticFlightControlLateralMode::RollAttitudeHold;
	capture_roll_reference();
	clear_lateral_degradation();
	++state_.lateral_guidance_reset_revision;
}

void AutopilotModeLogic::engage_heading_select()
{
	if (!observation_.magnetic_heading_available)
	{
		release_lateral();
		state_.lateral_degraded = true;
		state_.degradation_reason =
			DegradationReason::MagneticHeadingUnavailable;
		return;
	}
	initialize_heading_select();
	state_.lateral_mode = AutomaticFlightControlLateralMode::HeadingSelect;
	clear_lateral_degradation();
	++state_.lateral_guidance_reset_revision;
}

void AutopilotModeLogic::capture_roll_reference()
{
	state_.target_roll_rad = Common::limit(
		observation_.roll_rad, -config_.bank_limit_rad, config_.bank_limit_rad);
}

void AutopilotModeLogic::clear_vertical_degradation()
{
	state_.vertical_degraded = false;
	if (is_vertical_degradation(state_.degradation_reason))
		state_.degradation_reason = DegradationReason::None;
}

void AutopilotModeLogic::clear_lateral_degradation()
{
	state_.lateral_degraded = false;
	if (is_lateral_degradation(state_.degradation_reason))
		state_.degradation_reason = DegradationReason::None;
}

void AutopilotModeLogic::adjust_heading_select(int direction)
{
	initialize_heading_select();
	if (!heading_select_initialized_) return;
	state_.target_heading_deg = Common::wrap_heading_deg(
		state_.target_heading_deg +
			direction * config_.heading_select_step_deg);
}

void AutopilotModeLogic::initialize_heading_select()
{
	if (heading_select_initialized_ ||
		!observation_.magnetic_heading_available) return;
	state_.target_heading_deg = Common::wrap_heading_deg(
		static_cast<int>(std::lround(Common::wrap_heading_deg(
			observation_.magnetic_heading_deg))));
	heading_select_initialized_ = true;
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
	case AutomaticFlightControlVerticalMode::PitchAttitudeHold:
		engage_pitch_attitude_hold(); break;
	case AutomaticFlightControlVerticalMode::AltitudeHold:
		engage_altitude_hold(); break;
	default:
		break;
	}
}

void AutopilotModeLogic::recapture_lateral_reference()
{
	if (state_.lateral_mode ==
		AutomaticFlightControlLateralMode::RollAttitudeHold)
	{
		engage_roll_attitude_hold();
	}
	// Heading Select guidance tracks measured bank while bypassed. Keeping that
	// state lets its normal rate limiter resume authority without a command step.
}

void AutopilotModeLogic::update_pitch_stick_steering()
{
	const bool supported = state_.master_engaged &&
		state_.vertical_mode ==
			AutomaticFlightControlVerticalMode::PitchAttitudeHold &&
		!state_.bypass_active;
	const bool requested = supported &&
		std::abs(observation_.conditioned_pitch_input_normalized) >
			config_.pitch_stick_steering_threshold_normalized;
	if (requested)
	{
		state_.pitch_stick_steering_active = true;
		state_.target_pitch_rad = observation_.pitch_rad;
		++state_.vertical_guidance_reset_revision;
		return;
	}
	if (state_.pitch_stick_steering_active)
	{
		state_.target_pitch_rad = observation_.pitch_rad;
		++state_.vertical_guidance_reset_revision;
	}
	state_.pitch_stick_steering_active = false;
}

void AutopilotModeLogic::update_roll_stick_steering()
{
	const bool supported = state_.master_engaged &&
		state_.lateral_mode ==
			AutomaticFlightControlLateralMode::RollAttitudeHold &&
		!state_.bypass_active;
	const bool requested = supported &&
		std::abs(observation_.conditioned_roll_input_normalized) >
			config_.roll_stick_steering_threshold_normalized;
	if (requested || state_.roll_stick_steering_active)
	{
		capture_roll_reference();
		++state_.lateral_guidance_reset_revision;
	}
	state_.roll_stick_steering_active = requested;
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
		return;
	}
	if (state_.lateral_mode ==
		AutomaticFlightControlLateralMode::HeadingSelect &&
		!observation_.magnetic_heading_available)
	{
		release_lateral();
		state_.lateral_degraded = true;
		state_.degradation_reason =
			DegradationReason::MagneticHeadingUnavailable;
	}
	if (state_.vertical_mode ==
		AutomaticFlightControlVerticalMode::AltitudeHold &&
		!observation_.pressure_altitude_available)
	{
		release_vertical();
		state_.vertical_degraded = true;
		state_.degradation_reason =
			DegradationReason::PressureAltitudeUnavailable;
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
	++state_.vertical_guidance_reset_revision;
}

void AutopilotModeLogic::release_lateral()
{
	state_.lateral_mode = AutomaticFlightControlLateralMode::Off;
	state_.roll_stick_steering_active = false;
	++state_.lateral_guidance_reset_revision;
}

const AutopilotModeLogicState& AutopilotModeLogic::state() const
{
	return state_;
}
}
}
