#include "AutomaticFlightControl.h"

#include "../../SystemPipeline.h"

namespace
{
Core::Systems::AutopilotModeMonitorConfig monitor_config(
	const Core::Systems::AutomaticFlightControlConfig& config)
{
	return {
		config.pitch_tracking_error_limit_rad,
		config.vertical_speed_tracking_error_limit_mps,
		config.bank_tracking_error_limit_rad,
		config.tracking_failure_persistence_s,
		config.actuator_saturation_persistence_s
	};
}

Core::Systems::AuthorityState authority_state(
	bool axis_active,
	bool bypass_active,
	bool stick_steering_active)
{
	if (stick_steering_active)
		return Core::Systems::AuthorityState::StickSteering;
	if (bypass_active)
		return Core::Systems::AuthorityState::Bypassed;
	return axis_active
		? Core::Systems::AuthorityState::Automatic
		: Core::Systems::AuthorityState::Manual;
}
}

namespace Core
{
namespace Systems
{
AutomaticFlightControl::AutomaticFlightControl(
	const AutomaticFlightControlConfig& config,
	bool initial_weight_on_wheels)
	: config_(config),
	mode_logic_(config),
	vertical_guidance_(config),
	lateral_guidance_(config),
	auto_throttle_(config),
	mode_monitor_(monitor_config(config))
{
	validate_automatic_flight_control_config(config_);
	observation_.weight_on_wheels = initial_weight_on_wheels;
	snapshot_.status = { true, 0 };
	refresh_reference();
	refresh_snapshot();
}

void AutomaticFlightControl::register_commands(SystemSetup& setup)
{
	const CommandId commands[] = {
		CommandId::ToggleAutopilotMaster, CommandId::EngageAutopilot,
		CommandId::DisengageAutopilot, CommandId::SetAutopilotBypass,
		CommandId::SelectAutopilotPitchHold,
		CommandId::SelectAutopilotVerticalSpeedHold,
		CommandId::SelectAutopilotAltitudeHold,
		CommandId::IncreaseAutopilotVerticalReference,
		CommandId::DecreaseAutopilotVerticalReference,
		CommandId::SelectAutopilotHeadingHold,
		CommandId::SelectAutopilotHeading,
		CommandId::SelectAutopilotNavigationTrack,
		CommandId::IncreaseAutopilotLateralReference,
		CommandId::DecreaseAutopilotLateralReference,
		CommandId::ToggleAutoThrottle, CommandId::EngageAutoThrottle,
		CommandId::DisengageAutoThrottle,
		CommandId::IncreaseAutopilotSpeed,
		CommandId::DecreaseAutopilotSpeed
	};
	for (CommandId id : commands)
	{
		setup.register_command_handler(
			id,
			[this](const Command& command) { handle_command(command); });
	}
}

void AutomaticFlightControl::handle_command(const Command& command)
{
	if (AutopilotModeLogic::handles(command.id))
	{
		mode_logic_.handle_command(command);
		return;
	}
	pending_auto_throttle_commands_.push_back(command);
}

const AutomaticFlightGuidanceReference& AutomaticFlightControl::step(
	const AutomaticFlightControlObservation& observation)
{
	observation_ = observation;
	const bool previously_engaged = mode_logic_.state().master_engaged;
	mode_logic_.update(observation_, pending_monitor_result_);
	if (pending_monitor_result_.release_vertical)
		mode_monitor_.reset_vertical();
	if (pending_monitor_result_.release_lateral)
		mode_monitor_.reset_lateral();
	pending_monitor_result_ = {};
	apply_auto_throttle_commands();
	if (previously_engaged && !mode_logic_.state().master_engaged)
	{
		auto_throttle_.disengage(mode_logic_.state().disengage_reason);
		mode_monitor_.reset_all();
	}
	synchronize_guidance_lifecycle();
	update_guidance();
	(void)auto_throttle_.update(observation_);
	++revision_;
	refresh_reference();
	refresh_snapshot();
	return reference_;
}

void AutomaticFlightControl::apply_auto_throttle_commands()
{
	for (const Command& command : pending_auto_throttle_commands_)
	{
		(void)auto_throttle_.handle_command(command, observation_);
	}
	pending_auto_throttle_commands_.clear();
}

void AutomaticFlightControl::synchronize_guidance_lifecycle()
{
	const AutopilotModeLogicState& state = mode_logic_.state();
	if (applied_vertical_revision_ != state.vertical_reference_revision)
	{
		vertical_guidance_.reset();
		applied_vertical_revision_ = state.vertical_reference_revision;
	}
	if (applied_lateral_revision_ != state.lateral_reference_revision)
	{
		lateral_guidance_.reset();
		applied_lateral_revision_ = state.lateral_reference_revision;
	}
}

void AutomaticFlightControl::update_guidance()
{
	const AutopilotModeLogicState& state = mode_logic_.state();
	if (state.bypass_active)
	{
		vertical_reference_ = {};
		lateral_reference_ = {};
		return;
	}
	vertical_reference_ = vertical_guidance_.update(
		observation_,
		{ state.vertical_mode, state.target_pitch_rad,
			state.target_vertical_speed_mps, state.target_altitude_m },
		state.master_engaged);
	const bool lateral_active = state.master_engaged &&
		(state.lateral_mode == AutomaticFlightControlLateralMode::HeadingHold ||
			state.lateral_mode == AutomaticFlightControlLateralMode::HeadingSelect);
	const double target_heading_rad = state.lateral_mode ==
		AutomaticFlightControlLateralMode::HeadingSelect
		? state.target_heading_select_rad : state.target_heading_rad;
	lateral_reference_ = lateral_guidance_.update(
		observation_, target_heading_rad, lateral_active);
}

void AutomaticFlightControl::observe_control_result(
	const AutopilotModeMonitorObservation& observation)
{
	pending_monitor_result_ = mode_monitor_.update(observation);
	constraint_reason_ = pending_monitor_result_.constraint_reason;
	refresh_snapshot();
}

void AutomaticFlightControl::refresh_reference()
{
	const AutopilotModeLogicState& state = mode_logic_.state();
	const bool vertical_active = state.master_engaged &&
		state.vertical_mode != AutomaticFlightControlVerticalMode::Off;
	const bool lateral_active = state.master_engaged &&
		(state.lateral_mode == AutomaticFlightControlLateralMode::HeadingHold ||
			state.lateral_mode == AutomaticFlightControlLateralMode::HeadingSelect);
	const ExperimentalAutoThrottleResult& auto_throttle =
		auto_throttle_.result();
	reference_ = {
		authority_state(
			vertical_active, state.bypass_active,
			state.pitch_stick_steering_active),
		authority_state(lateral_active, state.bypass_active, false),
		vertical_reference_.type,
		vertical_reference_.pitch_attitude_rad,
		vertical_reference_.vertical_speed_mps,
		lateral_reference_.bank_angle_rad,
		0.0,
		auto_throttle.engaged,
		auto_throttle.throttle_normalized
	};
}

void AutomaticFlightControl::refresh_snapshot()
{
	const AutopilotModeLogicState& state = mode_logic_.state();
	const ExperimentalAutoThrottleResult& throttle = auto_throttle_.result();
	snapshot_.status = { true, revision_ };
	snapshot_.master_engaged = state.master_engaged;
	snapshot_.bypass_active = state.bypass_active;
	snapshot_.auto_throttle_engaged = throttle.engaged;
	snapshot_.vertical_mode = state.vertical_mode;
	snapshot_.lateral_mode = state.lateral_mode;
	snapshot_.pitch_attitude_reference_rad =
		vertical_reference_.pitch_attitude_rad;
	snapshot_.vertical_speed_reference_mps = vertical_reference_.vertical_speed_mps;
	snapshot_.bank_angle_reference_rad = lateral_reference_.bank_angle_rad;
	snapshot_.throttle_command_normalized = throttle.throttle_normalized;
	snapshot_.target_altitude_m = state.target_altitude_m;
	snapshot_.target_heading_rad = state.lateral_mode ==
		AutomaticFlightControlLateralMode::HeadingSelect
		? state.target_heading_select_rad : state.target_heading_rad;
	snapshot_.target_speed_mps = throttle.target_speed_mps;
	snapshot_.target_pitch_rad = state.target_pitch_rad;
	snapshot_.target_vertical_speed_mps = state.target_vertical_speed_mps;
	snapshot_.longitudinal_authority = reference_.longitudinal_authority;
	snapshot_.lateral_authority = reference_.lateral_authority;
	snapshot_.constraint_reason = constraint_reason_;
	snapshot_.degradation_reason = state.degradation_reason;
	snapshot_.disconnect_reason = state.disconnect_reason;
	snapshot_.vertical_degraded = state.vertical_degraded;
	snapshot_.lateral_degraded = state.lateral_degraded;
	snapshot_.autopilot_engage_rejection_reason = state.engage_rejection_reason;
	snapshot_.autopilot_disengage_reason = state.disengage_reason;
	snapshot_.auto_throttle_engage_rejection_reason =
		throttle.engage_rejection_reason;
	snapshot_.auto_throttle_disengage_reason = throttle.disengage_reason;
}

const AutomaticFlightControlSnapshot& AutomaticFlightControl::snapshot() const
{
	return snapshot_;
}
}
}
