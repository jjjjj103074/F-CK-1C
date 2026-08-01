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

}

namespace Core
{
namespace Systems
{
AutomaticFlightControl::AutomaticFlightControl(
	const AutomaticFlightControlConfig& config,
	bool initial_weight_on_wheels)
	: config_(config)
{
	validate_automatic_flight_control_config(config_);
	observation_.weight_on_wheels = initial_weight_on_wheels;
	snapshot_.status = { true, 0 };
	refresh_demand();
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
	(void)handle_auto_throttle_command(command);
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
			heading_integral_ = 0.0;
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

bool AutomaticFlightControl::handle_auto_throttle_command(
	const Command& command)
{
	if (!is_pressed(command)) return false;
	switch (command.id)
	{
	case CommandId::ToggleAutoThrottle:
		if (auto_throttle_engaged_)
			disengage_auto_throttle(AutomaticFlightControlReason::Commanded);
		else
			engage_auto_throttle();
		return true;
	case CommandId::EngageAutoThrottle:
		if (!auto_throttle_engaged_) engage_auto_throttle();
		return true;
	case CommandId::DisengageAutoThrottle:
		disengage_auto_throttle(AutomaticFlightControlReason::Commanded);
		return true;
	case CommandId::IncreaseAutopilotSpeed:
		if (auto_throttle_engaged_) adjust_speed_reference(1.0);
		return true;
	case CommandId::DecreaseAutopilotSpeed:
		if (auto_throttle_engaged_) adjust_speed_reference(-1.0);
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

bool AutomaticFlightControl::can_engage_auto_throttle()
{
	auto_throttle_rejection_reason_ = AutomaticFlightControlReason::None;
	if (observation_.mach > config_.maximum_auto_throttle_mach)
		auto_throttle_rejection_reason_ =
			AutomaticFlightControlReason::MachLimit;
	else if (observation_.weight_on_wheels)
		auto_throttle_rejection_reason_ =
			AutomaticFlightControlReason::WeightOnWheels;
	return auto_throttle_rejection_reason_ ==
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
	disengage_auto_throttle(reason);
	bypass_active_ = false;
}

void AutomaticFlightControl::engage_pitch_hold()
{
	vertical_mode_ = AutomaticFlightControlVerticalMode::PitchHold;
	target_pitch_rad_ = observation_.pitch_rad;
	vertical_speed_integral_ = 0.0;
}

void AutomaticFlightControl::engage_vertical_speed_hold()
{
	vertical_mode_ = AutomaticFlightControlVerticalMode::VerticalSpeedHold;
	target_vertical_speed_mps_ = observation_.vertical_speed_mps;
	vertical_speed_integral_ = 0.0;
}

void AutomaticFlightControl::engage_altitude_hold()
{
	vertical_mode_ = AutomaticFlightControlVerticalMode::AltitudeHold;
	target_altitude_m_ = observation_.altitude_m;
	vertical_speed_integral_ = 0.0;
}

void AutomaticFlightControl::engage_heading_hold()
{
	lateral_mode_ = AutomaticFlightControlLateralMode::HeadingHold;
	target_heading_rad_ = observation_.heading_rad;
	heading_integral_ = 0.0;
}

void AutomaticFlightControl::engage_heading_select()
{
	lateral_mode_ = AutomaticFlightControlLateralMode::HeadingSelect;
	if (!heading_select_initialized_)
	{
		target_heading_select_rad_ = observation_.heading_rad;
		heading_select_initialized_ = true;
	}
	heading_integral_ = 0.0;
}

void AutomaticFlightControl::engage_auto_throttle()
{
	if (!can_engage_auto_throttle()) return;
	auto_throttle_engaged_ = true;
	auto_throttle_disengage_reason_ = AutomaticFlightControlReason::None;
	target_speed_mps_ = observation_.indicated_airspeed_mps;
	speed_integral_ = 0.0;
}

void AutomaticFlightControl::disengage_auto_throttle(
	AutomaticFlightControlReason reason)
{
	if (auto_throttle_engaged_ || reason == AutomaticFlightControlReason::Commanded)
		auto_throttle_disengage_reason_ = reason;
	reset_auto_throttle_controller();
}

void AutomaticFlightControl::adjust_vertical_reference(double direction)
{
	switch (vertical_mode_)
	{
	case AutomaticFlightControlVerticalMode::AltitudeHold:
		target_altitude_m_ += direction * config_.altitude_step_m;
		vertical_speed_integral_ = 0.0;
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

void AutomaticFlightControl::adjust_speed_reference(double direction)
{
	target_speed_mps_ = Common::limit(
		target_speed_mps_ + direction * config_.speed_step_mps,
		config_.minimum_target_speed_mps,
		config_.maximum_target_speed_mps);
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
	bypass_start_pitch_rad_ = observation_.pitch_rad;
	bypass_start_roll_rad_ = observation_.roll_rad;
	bypass_start_heading_rad_ = observation_.heading_rad;
	pitch_command_ = 0.0;
	roll_command_ = 0.0;
}

void AutomaticFlightControl::exit_bypass()
{
	if (!bypass_active_) return;
	bypass_active_ = false;
	if (bypass_change_is_meaningful())
	{
		recapture_vertical_reference();
		recapture_lateral_reference();
	}
}

bool AutomaticFlightControl::bypass_change_is_meaningful() const
{
	const double pitch_change =
		std::abs(observation_.pitch_rad - bypass_start_pitch_rad_);
	const double roll_change =
		std::abs(observation_.roll_rad - bypass_start_roll_rad_);
	const double heading_change = std::abs(wrap_pi(
		observation_.heading_rad - bypass_start_heading_rad_));
	return pitch_change > config_.bypass_attitude_threshold_rad ||
		roll_change > config_.bypass_attitude_threshold_rad ||
		heading_change > config_.bypass_attitude_threshold_rad;
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

const AutomaticFlightControlDemand& AutomaticFlightControl::step(
	const AutomaticFlightControlObservation& observation)
{
	observation_ = observation;
	apply_pending_commands();
	apply_disconnect_guards();
	if (bypass_active_)
	{
		pitch_command_ = 0.0;
		roll_command_ = 0.0;
	}
	else
	{
		update_vertical_controller();
		update_lateral_controller();
	}
	update_auto_throttle();
	++revision_;
	refresh_demand();
	refresh_snapshot();
	return demand_;
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
	if (auto_throttle_engaged_ &&
		observation_.mach > config_.auto_throttle_disconnect_mach)
	{
		disengage_auto_throttle(AutomaticFlightControlReason::MachLimit);
	}
}

void AutomaticFlightControl::reset_vertical_controller()
{
	vertical_mode_ = AutomaticFlightControlVerticalMode::Off;
	pitch_command_ = 0.0;
	vertical_speed_integral_ = 0.0;
}

void AutomaticFlightControl::reset_lateral_controller()
{
	lateral_mode_ = AutomaticFlightControlLateralMode::Off;
	roll_command_ = 0.0;
	heading_integral_ = 0.0;
}

void AutomaticFlightControl::reset_auto_throttle_controller()
{
	auto_throttle_engaged_ = false;
	throttle_command_ = 0.0;
	speed_integral_ = 0.0;
}

void AutomaticFlightControl::refresh_demand()
{
	demand_ = {
		master_engaged_ && !bypass_active_,
		auto_throttle_engaged_,
		pitch_command_,
		roll_command_,
		throttle_command_
	};
}

void AutomaticFlightControl::refresh_snapshot()
{
	snapshot_.status = { true, revision_ };
	snapshot_.master_engaged = master_engaged_;
	snapshot_.bypass_active = bypass_active_;
	snapshot_.auto_throttle_engaged = auto_throttle_engaged_;
	snapshot_.vertical_mode = vertical_mode_;
	snapshot_.lateral_mode = lateral_mode_;
	snapshot_.pitch_command_normalized = pitch_command_;
	snapshot_.roll_command_normalized = roll_command_;
	snapshot_.throttle_command_normalized = throttle_command_;
	snapshot_.target_altitude_m = target_altitude_m_;
	snapshot_.target_heading_rad =
		lateral_mode_ == AutomaticFlightControlLateralMode::HeadingSelect
		? target_heading_select_rad_ : target_heading_rad_;
	snapshot_.target_speed_mps = target_speed_mps_;
	snapshot_.target_pitch_rad = target_pitch_rad_;
	snapshot_.target_vertical_speed_mps = target_vertical_speed_mps_;
	snapshot_.autopilot_engage_rejection_reason =
		autopilot_rejection_reason_;
	snapshot_.autopilot_disengage_reason =
		autopilot_disengage_reason_;
	snapshot_.auto_throttle_engage_rejection_reason =
		auto_throttle_rejection_reason_;
	snapshot_.auto_throttle_disengage_reason =
		auto_throttle_disengage_reason_;
}

const AutomaticFlightControlSnapshot&
AutomaticFlightControl::snapshot() const
{
	return snapshot_;
}
}
}
