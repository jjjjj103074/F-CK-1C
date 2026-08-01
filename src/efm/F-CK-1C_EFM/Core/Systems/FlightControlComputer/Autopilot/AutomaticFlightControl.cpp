#include "AutomaticFlightControl.h"

#include "../../SystemPipeline.h"
#include "Common/Clamp.h"
#include "Common/Units.h"

#include <cmath>

namespace
{
constexpr double kEnabledCommandThreshold = 0.5;
constexpr double kTwo = 2.0;

bool is_pressed(const Core::Command& command)
{
	return command.value > kEnabledCommandThreshold;
}

double wrap_pi(double angle)
{
	const double period = kTwo * Common::kPi;
	while (angle > Common::kPi) angle -= period;
	while (angle < -Common::kPi) angle += period;
	return angle;
}

double wrap_two_pi(double angle)
{
	const double period = kTwo * Common::kPi;
	while (angle >= period) angle -= period;
	while (angle < 0.0) angle += period;
	return angle;
}

Core::Systems::AuthorityState authority_state(
	bool master_engaged,
	bool bypass_active,
	bool stick_steering_active)
{
	if (stick_steering_active)
		return Core::Systems::AuthorityState::StickSteering;
	if (bypass_active)
		return Core::Systems::AuthorityState::Bypassed;
	return master_engaged
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
	vertical_guidance_(config),
	lateral_guidance_(config),
	auto_throttle_(config)
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
	pending_commands_.push_back(command);
}

void AutomaticFlightControl::apply_pending_commands()
{
	for (const Command& command : pending_commands_)
	{
		apply_command(command);
	}
	pending_commands_.clear();
}

void AutomaticFlightControl::apply_command(const Command& command)
{
	if (handle_master_command(command)) return;
	if (handle_vertical_command(command)) return;
	if (handle_lateral_command(command)) return;
	(void)auto_throttle_.handle_command(command, observation_);
}

bool AutomaticFlightControl::handle_master_command(const Command& command)
{
	switch (command.id)
	{
	case CommandId::ToggleAutopilotMaster:
		if (!is_pressed(command)) return true;
		if (master_engaged_)
			disengage_all(AutomaticFlightControlReason::Commanded);
		else
			engage_autopilot();
		return true;
	case CommandId::EngageAutopilot:
		if (is_pressed(command) && !master_engaged_) engage_autopilot();
		return true;
	case CommandId::DisengageAutopilot:
		if (is_pressed(command))
			disengage_all(AutomaticFlightControlReason::Commanded);
		return true;
	case CommandId::SetAutopilotBypass:
		set_bypass(is_pressed(command));
		return true;
	default:
		return false;
	}
}

bool AutomaticFlightControl::handle_vertical_command(const Command& command)
{
	if (!is_pressed(command)) return false;
	switch (command.id)
	{
	case CommandId::SelectAutopilotPitchHold:
		if (master_engaged_) engage_pitch_hold();
		return true;
	case CommandId::SelectAutopilotVerticalSpeedHold:
		if (master_engaged_) engage_vertical_speed_hold();
		return true;
	case CommandId::SelectAutopilotAltitudeHold:
		if (master_engaged_) engage_altitude_hold();
		return true;
	case CommandId::IncreaseAutopilotVerticalReference:
		if (master_engaged_) adjust_vertical_reference(1.0);
		return true;
	case CommandId::DecreaseAutopilotVerticalReference:
		if (master_engaged_) adjust_vertical_reference(-1.0);
		return true;
	default:
		return false;
	}
}

bool AutomaticFlightControl::handle_lateral_command(const Command& command)
{
	if (!is_pressed(command)) return false;
	switch (command.id)
	{
	case CommandId::SelectAutopilotHeadingHold:
		if (master_engaged_) engage_heading_hold();
		return true;
	case CommandId::SelectAutopilotHeading:
		if (master_engaged_) engage_heading_select();
		return true;
	case CommandId::SelectAutopilotNavigationTrack:
		if (master_engaged_)
		{
			lateral_mode_ = AutomaticFlightControlLateralMode::NavigationTrack;
			lateral_guidance_.reset();
		}
		return true;
	case CommandId::IncreaseAutopilotLateralReference:
		if (master_engaged_) adjust_lateral_reference(1.0);
		return true;
	case CommandId::DecreaseAutopilotLateralReference:
		if (master_engaged_) adjust_lateral_reference(-1.0);
		return true;
	default:
		return false;
	}
}

bool AutomaticFlightControl::can_engage_autopilot()
{
	autopilot_rejection_reason_ = AutomaticFlightControlReason::None;
	if (observation_.indicated_airspeed_mps < config_.minimum_ias_mps)
		autopilot_rejection_reason_ =
			AutomaticFlightControlReason::BelowMinimumIndicatedAirspeed;
	else if (observation_.weight_on_wheels)
		autopilot_rejection_reason_ =
			AutomaticFlightControlReason::WeightOnWheels;
	else if (std::abs(observation_.roll_rad) > config_.engage_roll_limit_rad)
		autopilot_rejection_reason_ =
			AutomaticFlightControlReason::RollLimit;
	else if (std::abs(observation_.pitch_rad) > config_.engage_pitch_limit_rad)
		autopilot_rejection_reason_ =
			AutomaticFlightControlReason::PitchLimit;
	return autopilot_rejection_reason_ ==
		AutomaticFlightControlReason::None;
}

void AutomaticFlightControl::engage_autopilot()
{
	if (!can_engage_autopilot()) return;
	master_engaged_ = true;
	autopilot_disengage_reason_ = AutomaticFlightControlReason::None;
	engage_pitch_hold();
	engage_heading_hold();
}

void AutomaticFlightControl::disengage_all(
	AutomaticFlightControlReason reason)
{
	master_engaged_ = false;
	autopilot_disengage_reason_ = reason;
	reset_vertical_controller();
	reset_lateral_controller();
	auto_throttle_.disengage(reason);
	bypass_active_ = false;
	pitch_stick_steering_active_ = false;
}

void AutomaticFlightControl::engage_pitch_hold()
{
	vertical_mode_ = AutomaticFlightControlVerticalMode::PitchHold;
	target_pitch_rad_ = observation_.pitch_rad;
	vertical_guidance_.reset();
}

void AutomaticFlightControl::engage_vertical_speed_hold()
{
	vertical_mode_ = AutomaticFlightControlVerticalMode::VerticalSpeedHold;
	target_vertical_speed_mps_ = observation_.vertical_speed_mps;
	vertical_guidance_.reset();
}

void AutomaticFlightControl::engage_altitude_hold()
{
	vertical_mode_ = AutomaticFlightControlVerticalMode::AltitudeHold;
	target_altitude_m_ = observation_.altitude_m;
	vertical_guidance_.reset();
}

void AutomaticFlightControl::engage_heading_hold()
{
	lateral_mode_ = AutomaticFlightControlLateralMode::HeadingHold;
	target_heading_rad_ = observation_.heading_rad;
	lateral_guidance_.reset();
}

void AutomaticFlightControl::engage_heading_select()
{
	lateral_mode_ = AutomaticFlightControlLateralMode::HeadingSelect;
	if (!heading_select_initialized_)
	{
		target_heading_select_rad_ = observation_.heading_rad;
		heading_select_initialized_ = true;
	}
	lateral_guidance_.reset();
}

void AutomaticFlightControl::adjust_vertical_reference(double direction)
{
	switch (vertical_mode_)
	{
	case AutomaticFlightControlVerticalMode::AltitudeHold:
		target_altitude_m_ += direction * config_.altitude_step_m;
		vertical_guidance_.reset();
		break;
	case AutomaticFlightControlVerticalMode::VerticalSpeedHold:
		target_vertical_speed_mps_ +=
			direction * config_.vertical_speed_step_mps;
		break;
	case AutomaticFlightControlVerticalMode::PitchHold:
		target_pitch_rad_ += direction * config_.pitch_step_rad;
		break;
	default:
		break;
	}
}

void AutomaticFlightControl::adjust_lateral_reference(double direction)
{
	if (lateral_mode_ == AutomaticFlightControlLateralMode::HeadingHold)
	{
		target_heading_rad_ = wrap_two_pi(
			target_heading_rad_ + direction * config_.heading_step_rad);
	}
	if (lateral_mode_ == AutomaticFlightControlLateralMode::HeadingSelect)
	{
		target_heading_select_rad_ = wrap_two_pi(
			target_heading_select_rad_ + direction * config_.heading_step_rad);
	}
}

void AutomaticFlightControl::set_bypass(bool requested)
{
	if (requested)
	{
		enter_bypass();
		return;
	}
	exit_bypass();
}

void AutomaticFlightControl::enter_bypass()
{
	if (!master_engaged_ || bypass_active_) return;
	bypass_active_ = true;
	vertical_reference_ = {};
	lateral_reference_ = {};
}

void AutomaticFlightControl::exit_bypass()
{
	if (!bypass_active_) return;
	bypass_active_ = false;
	recapture_vertical_reference();
	recapture_lateral_reference();
}

void AutomaticFlightControl::recapture_vertical_reference()
{
	switch (vertical_mode_)
	{
	case AutomaticFlightControlVerticalMode::PitchHold:
		engage_pitch_hold();
		break;
	case AutomaticFlightControlVerticalMode::VerticalSpeedHold:
		engage_vertical_speed_hold();
		break;
	case AutomaticFlightControlVerticalMode::AltitudeHold:
		engage_altitude_hold();
		break;
	default:
		break;
	}
}

void AutomaticFlightControl::recapture_lateral_reference()
{
	if (lateral_mode_ == AutomaticFlightControlLateralMode::HeadingHold)
	{
		engage_heading_hold();
	}
}

const AutomaticFlightGuidanceReference& AutomaticFlightControl::step(
	const AutomaticFlightControlObservation& observation)
{
	observation_ = observation;
	apply_pending_commands();
	apply_disconnect_guards();
	update_pitch_stick_steering();
	if (bypass_active_)
	{
		vertical_reference_ = {};
		lateral_reference_ = {};
	}
	else
	{
		vertical_reference_ = vertical_guidance_.update(
			observation_,
			{ vertical_mode_,
				target_pitch_rad_,
				target_vertical_speed_mps_,
				target_altitude_m_ },
			master_engaged_);
		const bool lateral_active =
			master_engaged_ &&
			(lateral_mode_ == AutomaticFlightControlLateralMode::HeadingHold ||
				lateral_mode_ == AutomaticFlightControlLateralMode::HeadingSelect);
		const double target_heading =
			lateral_mode_ == AutomaticFlightControlLateralMode::HeadingSelect
			? target_heading_select_rad_ : target_heading_rad_;
		lateral_reference_ = lateral_guidance_.update(
			observation_, target_heading, lateral_active);
	}
	(void)auto_throttle_.update(observation_);
	++revision_;
	refresh_reference();
	refresh_snapshot();
	return reference_;
}

void AutomaticFlightControl::update_pitch_stick_steering()
{
	const bool supported_mode =
		master_engaged_ &&
		vertical_mode_ == AutomaticFlightControlVerticalMode::PitchHold &&
		!bypass_active_;
	const bool requested = supported_mode &&
		std::abs(observation_.conditioned_pitch_input_normalized) >
			config_.pitch_stick_steering_threshold_normalized;
	if (requested)
	{
		pitch_stick_steering_active_ = true;
		target_pitch_rad_ = observation_.pitch_rad;
		vertical_guidance_.reset();
		return;
	}
	if (pitch_stick_steering_active_)
	{
		target_pitch_rad_ = observation_.pitch_rad;
		vertical_guidance_.reset();
	}
	pitch_stick_steering_active_ = false;
}

void AutomaticFlightControl::apply_disconnect_guards()
{
	if (master_engaged_ &&
		observation_.indicated_airspeed_mps < config_.minimum_ias_mps)
	{
		disengage_all(
			AutomaticFlightControlReason::BelowMinimumIndicatedAirspeed);
		return;
	}
	if (master_engaged_ && observation_.weight_on_wheels)
	{
		disengage_all(AutomaticFlightControlReason::WeightOnWheels);
		return;
	}
}

void AutomaticFlightControl::reset_vertical_controller()
{
	vertical_mode_ = AutomaticFlightControlVerticalMode::Off;
	vertical_reference_ = {};
	vertical_guidance_.reset();
	pitch_stick_steering_active_ = false;
}

void AutomaticFlightControl::reset_lateral_controller()
{
	lateral_mode_ = AutomaticFlightControlLateralMode::Off;
	lateral_reference_ = {};
	lateral_guidance_.reset();
}

void AutomaticFlightControl::refresh_reference()
{
	const ExperimentalAutoThrottleResult& auto_throttle =
		auto_throttle_.result();
	reference_ = {
		authority_state(
			master_engaged_, bypass_active_, pitch_stick_steering_active_),
		authority_state(master_engaged_, bypass_active_, false),
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
	snapshot_.status = { true, revision_ };
	snapshot_.master_engaged = master_engaged_;
	snapshot_.bypass_active = bypass_active_;
	const ExperimentalAutoThrottleResult& auto_throttle =
		auto_throttle_.result();
	snapshot_.auto_throttle_engaged = auto_throttle.engaged;
	snapshot_.vertical_mode = vertical_mode_;
	snapshot_.lateral_mode = lateral_mode_;
	snapshot_.pitch_attitude_reference_rad =
		vertical_reference_.pitch_attitude_rad;
	snapshot_.vertical_speed_reference_mps =
		vertical_reference_.vertical_speed_mps;
	snapshot_.bank_angle_reference_rad =
		lateral_reference_.bank_angle_rad;
	snapshot_.throttle_command_normalized = auto_throttle.throttle_normalized;
	snapshot_.target_altitude_m = target_altitude_m_;
	snapshot_.target_heading_rad =
		lateral_mode_ == AutomaticFlightControlLateralMode::HeadingSelect
		? target_heading_select_rad_ : target_heading_rad_;
	snapshot_.target_speed_mps = auto_throttle.target_speed_mps;
	snapshot_.target_pitch_rad = target_pitch_rad_;
	snapshot_.target_vertical_speed_mps = target_vertical_speed_mps_;
	snapshot_.longitudinal_authority = reference_.longitudinal_authority;
	snapshot_.lateral_authority = reference_.lateral_authority;
	snapshot_.autopilot_engage_rejection_reason =
		autopilot_rejection_reason_;
	snapshot_.autopilot_disengage_reason =
		autopilot_disengage_reason_;
	snapshot_.auto_throttle_engage_rejection_reason =
		auto_throttle.engage_rejection_reason;
	snapshot_.auto_throttle_disengage_reason =
		auto_throttle.disengage_reason;
}

const AutomaticFlightControlSnapshot&
AutomaticFlightControl::snapshot() const
{
	return snapshot_;
}
}
}
