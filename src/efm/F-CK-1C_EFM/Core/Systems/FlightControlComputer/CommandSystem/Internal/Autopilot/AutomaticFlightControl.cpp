#include "AutomaticFlightControl.h"

#include "Common/Units.h"
#include "../../../Diagnostics/FlightControlSnapshotProjection.h"
#include "../../../../SystemPipeline.h"

#include <stdexcept>

namespace
{
Core::Systems::AutopilotModeMonitorConfig monitor_config(
	const Core::Systems::AutomaticFlightControlConfig& config)
{
	return {
		config.pitch_tracking_error_limit_rad,
		config.vertical_speed_tracking_error_limit_ft_s,
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

bool lateral_guidance_available(
	Core::Systems::AutomaticFlightControlLateralMode mode)
{
	return mode == Core::Systems::AutomaticFlightControlLateralMode::
			RollAttitudeHold ||
		mode == Core::Systems::AutomaticFlightControlLateralMode::HeadingSelect;
}

bool auto_throttle_command(Core::CommandId id)
{
	switch (id)
	{
	case Core::CommandId::ToggleAutoThrottle:
	case Core::CommandId::EngageAutoThrottle:
	case Core::CommandId::DisengageAutoThrottle:
	case Core::CommandId::IncreaseAutopilotSpeed:
	case Core::CommandId::DecreaseAutopilotSpeed:
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
AutomaticFlightControl::AutomaticFlightControl(
	const AutomaticFlightControlConfig& config,
	bool initial_weight_on_wheels,
	bool experimental_auto_throttle_available)
	: config_(config),
	mode_logic_(config),
	vertical_guidance_(config),
	lateral_guidance_(config),
	auto_throttle_(config.experimental_auto_throttle),
	mode_monitor_(monitor_config(config)),
	experimental_auto_throttle_available_(
		experimental_auto_throttle_available)
{
	validate_automatic_flight_control_config(config_);
	observation_.weight_on_wheels = initial_weight_on_wheels;
	snapshot_.status = { true, 0 };
	refresh_reference();
	refresh_snapshot();
}

bool AutomaticFlightControl::handles(CommandId id)
{
	if (AutopilotModeLogic::handles(id)) return true;
	switch (id)
	{
	case CommandId::ToggleAutoThrottle:
	case CommandId::EngageAutoThrottle:
	case CommandId::DisengageAutoThrottle:
	case CommandId::IncreaseAutopilotSpeed:
	case CommandId::DecreaseAutopilotSpeed:
		return true;
	default:
		return false;
	}
}

void AutomaticFlightControl::register_commands(SystemSetup& setup)
{
	const CommandId commands[] = {
		CommandId::EngageAutopilot, CommandId::DisengageAutopilot,
		CommandId::SetAutopilotBypass,
		CommandId::SelectAutopilotPitchAttitudeHold,
		CommandId::SelectAutopilotAltitudeHold,
		CommandId::SelectAutopilotRollAttitudeHold,
		CommandId::SelectAutopilotHeadingSelect,
		CommandId::IncreaseAutopilotHeadingSelect,
		CommandId::DecreaseAutopilotHeadingSelect
	};
	for (CommandId id : commands)
	{
		setup.register_command_handler(
			id,
			[this](const SystemActionContext&, const Command& command)
			{ handle_command(command); });
	}
	if (!experimental_auto_throttle_available_) return;
	const CommandId auto_throttle_commands[] = {
		CommandId::ToggleAutoThrottle, CommandId::EngageAutoThrottle,
		CommandId::DisengageAutoThrottle,
		CommandId::IncreaseAutopilotSpeed,
		CommandId::DecreaseAutopilotSpeed
	};
	for (CommandId id : auto_throttle_commands)
	{
		setup.register_command_handler(
			id,
			[this](const SystemActionContext&, const Command& command)
			{ handle_command(command); });
	}
}

void AutomaticFlightControl::handle_command(const Command& command)
{
	if (AutopilotModeLogic::handles(command.id))
	{
		mode_logic_.handle_command(command);
		return;
	}
	if (auto_throttle_command(command.id) &&
		!experimental_auto_throttle_available_)
	{
		throw std::logic_error(
			"Experimental auto-throttle is disabled by configuration.");
	}
	pending_auto_throttle_commands_.push_back(command);
}

const AutomaticFlightGuidanceReference& AutomaticFlightControl::step(
	const AutomaticFlightControlObservation& observation,
	const ::Systems::ManeuverEnvelope& envelope)
{
	observation_ = observation;
	const bool previously_engaged = mode_logic_.state().master_engaged;
	mode_logic_.update(observation_, pending_monitor_result_, envelope);
	apply_auto_throttle_commands();
	if (previously_engaged && !mode_logic_.state().master_engaged)
	{
		auto_throttle_.disengage(mode_logic_.state().disengage_reason);
		mode_monitor_.reset_all();
	}
	synchronize_guidance_lifecycle();
	update_guidance();
	pending_monitor_result_ = {};
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
	if (!state.master_engaged || state.bypass_active)
	{
		mode_monitor_.reset_all();
		pending_monitor_result_.vertical_constrained = false;
		pending_monitor_result_.lateral_constrained = false;
	}
	if (applied_vertical_guidance_reset_revision_ !=
		state.vertical_guidance_reset_revision)
	{
		vertical_guidance_.reset();
		mode_monitor_.reset_vertical();
		mode_monitor_.reset_saturation();
		pending_monitor_result_.vertical_constrained = false;
		applied_vertical_guidance_reset_revision_ =
			state.vertical_guidance_reset_revision;
	}
	if (applied_lateral_guidance_reset_revision_ !=
		state.lateral_guidance_reset_revision)
	{
		lateral_guidance_.reset();
		mode_monitor_.reset_lateral();
		mode_monitor_.reset_saturation();
		pending_monitor_result_.lateral_constrained = false;
		applied_lateral_guidance_reset_revision_ =
			state.lateral_guidance_reset_revision;
	}
}

void AutomaticFlightControl::update_guidance()
{
	const AutopilotModeLogicState& state = mode_logic_.state();
	if (state.bypass_active)
	{
		lateral_guidance_.track_observation(observation_);
		lateral_bypass_active_last_tick_ = true;
		vertical_reference_ = {};
		lateral_reference_ = {};
		return;
	}
	if (lateral_bypass_active_last_tick_)
	{
		lateral_guidance_.track_observation(observation_);
		lateral_bypass_active_last_tick_ = false;
	}
	vertical_reference_ = vertical_guidance_.update(
		observation_,
		{ state.vertical_mode, state.target_pitch_rad,
			state.target_altitude_ft },
		state.master_engaged);
	const bool lateral_active = state.master_engaged &&
		lateral_guidance_available(state.lateral_mode);
	lateral_reference_ = lateral_guidance_.update(
		{ observation_, state.lateral_mode,
			static_cast<double>(state.target_heading_deg),
			state.target_roll_rad, lateral_active,
			pending_monitor_result_.lateral_constrained });
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
		lateral_guidance_available(state.lateral_mode);
	const ExperimentalAutoThrottleResult& auto_throttle =
		auto_throttle_.result();
	reference_ = {
		authority_state(
			vertical_active, state.bypass_active,
			state.pitch_stick_steering_active),
		authority_state(
			lateral_active, state.bypass_active,
			state.roll_stick_steering_active),
		vertical_reference_.type,
		vertical_reference_.pitch_attitude_rad,
		vertical_reference_.vertical_speed_ft_s,
		lateral_reference_.bank_angle_rad,
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
	snapshot_.experimental_auto_throttle_available =
		experimental_auto_throttle_available_;
	snapshot_.auto_throttle_engaged = throttle.engaged;
	snapshot_.vertical_mode = SnapshotProjection::vertical_mode(
		state.vertical_mode);
	snapshot_.lateral_mode = SnapshotProjection::lateral_mode(
		state.lateral_mode);
	snapshot_.pitch_attitude_reference_rad =
		vertical_reference_.pitch_attitude_rad;
	snapshot_.vertical_speed_reference_ft_s =
		vertical_reference_.vertical_speed_ft_s;
	snapshot_.bank_angle_reference_rad = lateral_reference_.bank_angle_rad;
	snapshot_.throttle_command_normalized = throttle.throttle_normalized;
	snapshot_.target_altitude_ft = state.target_altitude_ft;
	snapshot_.target_heading_deg = state.target_heading_deg;
	snapshot_.target_speed_mps = throttle.target_speed_mps;
	snapshot_.target_pitch_rad = state.target_pitch_rad;
	snapshot_.longitudinal_authority =
		SnapshotProjection::authority(reference_.longitudinal_authority);
	snapshot_.lateral_authority =
		SnapshotProjection::authority(reference_.lateral_authority);
	snapshot_.constraint_reason =
		SnapshotProjection::constraint(constraint_reason_);
	snapshot_.degradation_reason =
		SnapshotProjection::degradation(state.degradation_reason);
	snapshot_.disconnect_reason =
		SnapshotProjection::disconnect(state.disconnect_reason);
	snapshot_.vertical_degraded = state.vertical_degraded;
	snapshot_.lateral_degraded = state.lateral_degraded;
	snapshot_.autopilot_engage_rejection_reason =
		SnapshotProjection::automatic_reason(state.engage_rejection_reason);
	snapshot_.autopilot_disengage_reason =
		SnapshotProjection::automatic_reason(state.disengage_reason);
	snapshot_.auto_throttle_engage_rejection_reason =
		SnapshotProjection::automatic_reason(
			throttle.engage_rejection_reason);
	snapshot_.auto_throttle_disengage_reason =
		SnapshotProjection::automatic_reason(throttle.disengage_reason);
}

const AutomaticFlightControlSnapshot& AutomaticFlightControl::snapshot() const
{
	return snapshot_;
}
}
}
