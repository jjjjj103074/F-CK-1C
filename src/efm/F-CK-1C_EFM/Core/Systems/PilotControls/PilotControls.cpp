#include "PilotControls.h"

#include "../SystemPipeline.h"
#include "../SystemUpdateRates.h"

namespace Core
{
namespace Systems
{
PilotControls::PilotControls(
	const ThrottleLeverSignal& initial_throttle_levers)
{
	::Systems::reset_throttle_inputs(
		throttle_inputs_,
		initial_throttle_levers.left_normalized,
		initial_throttle_levers.right_normalized);
	refresh_signal();
}

void PilotControls::setup(SystemSetup& setup)
{
	setup.update_rate_hz(kProjectDefinedFallbackUpdateRateHz);
	setup.publish(AircraftDataKeys::kPilotControlSignal, signal_);
	setup.publish(AircraftDataKeys::kThrottleLeverSignal, throttle_levers_);
	register_commands(setup);
}

void PilotControls::register_commands(SystemSetup& setup)
{
	const CommandId commands[] = {
		CommandId::SetPitchAxis, CommandId::SetPitchDiscrete,
		CommandId::AdjustPitchTrim, CommandId::SetRollAxis,
		CommandId::SetRollDiscrete, CommandId::AdjustRollTrim,
		CommandId::SetYawAxis, CommandId::SetYawDiscrete,
		CommandId::AdjustYawTrim, CommandId::ResetTrim,
		CommandId::SetCommonThrottleAxis, CommandId::SetLeftThrottleAxis,
		CommandId::SetRightThrottleAxis, CommandId::StepCommonThrottle,
		CommandId::StepLeftThrottle, CommandId::StepRightThrottle
	};
	for (CommandId id : commands)
	{
		setup.register_command_handler(
			id,
			[this](const SystemActionContext&, const Command& command)
			{ handle_command(command); });
	}
}

void PilotControls::step(
	const SystemStepContext& context,
	const AircraftDataView& aircraft,
	SystemResult& result)
{
	(void)aircraft;
	primary_controls_ = ::Systems::update_primary_control_inputs(
		primary_controls_, context.dt_s);
	::Systems::update_pilot_throttle_cmds(throttle_inputs_);
	refresh_signal();
	result.publish(AircraftDataKeys::kPilotControlSignal, signal_);
	result.publish(AircraftDataKeys::kThrottleLeverSignal, throttle_levers_);
}

void PilotControls::handle_command(const Command& command)
{
	handle_pitch_roll_command(command);
	handle_yaw_command(command);
	handle_throttle_command(command);
	refresh_signal();
}

void PilotControls::handle_pitch_roll_command(const Command& command)
{
	switch (command.id)
	{
	case CommandId::SetPitchAxis:
		::Systems::set_pitch_axis_input(
			primary_controls_, command.value_normalized); break;
	case CommandId::SetPitchDiscrete:
		::Systems::set_pitch_discrete_input(
			primary_controls_, static_cast<int>(command.value_normalized)); break;
	case CommandId::AdjustPitchTrim:
		::Systems::adjust_pitch_trim(
			primary_controls_, command.value_normalized); break;
	case CommandId::SetRollAxis:
		::Systems::set_roll_axis_input(
			primary_controls_, command.value_normalized); break;
	case CommandId::SetRollDiscrete:
		::Systems::set_roll_discrete_input(
			primary_controls_, static_cast<int>(command.value_normalized)); break;
	case CommandId::AdjustRollTrim:
		::Systems::adjust_roll_trim(
			primary_controls_, command.value_normalized); break;
	default:
		break;
	}
}

void PilotControls::handle_yaw_command(const Command& command)
{
	switch (command.id)
	{
	case CommandId::SetYawAxis:
		::Systems::set_yaw_axis_input(
			primary_controls_, command.value_normalized); break;
	case CommandId::SetYawDiscrete:
		::Systems::set_yaw_discrete_input(
			primary_controls_, static_cast<int>(command.value_normalized)); break;
	case CommandId::AdjustYawTrim:
		::Systems::adjust_yaw_trim(
			primary_controls_, command.value_normalized); break;
	case CommandId::ResetTrim:
		::Systems::reset_primary_trims(primary_controls_); break;
	default:
		break;
	}
}

void PilotControls::handle_throttle_command(const Command& command)
{
	switch (command.id)
	{
	case CommandId::SetCommonThrottleAxis:
		::Systems::set_common_throttle_axis(
			throttle_inputs_, command.value_normalized); break;
	case CommandId::SetLeftThrottleAxis:
		::Systems::set_left_throttle_axis(
			throttle_inputs_, command.value_normalized); break;
	case CommandId::SetRightThrottleAxis:
		::Systems::set_right_throttle_axis(
			throttle_inputs_, command.value_normalized); break;
	case CommandId::StepCommonThrottle:
		::Systems::step_common_keyboard_throttle(
			throttle_inputs_, command.value_normalized); break;
	case CommandId::StepLeftThrottle:
		::Systems::step_left_keyboard_throttle(
			throttle_inputs_, command.value_normalized); break;
	case CommandId::StepRightThrottle:
		::Systems::step_right_keyboard_throttle(
			throttle_inputs_, command.value_normalized); break;
	default:
		break;
	}
}

void PilotControls::refresh_signal()
{
	signal_ = {
		primary_controls_.pitch.input,
		primary_controls_.roll.input,
		primary_controls_.yaw.input,
		primary_controls_.pitch.trim,
		primary_controls_.roll.trim,
		primary_controls_.yaw.trim
	};
	throttle_levers_ = {
		throttle_inputs_.left.pilot_cmd,
		throttle_inputs_.right.pilot_cmd
	};
}

const PilotControlSignal& PilotControls::signal() const
{
	return signal_;
}

const ThrottleLeverSignal& PilotControls::throttle_levers() const
{
	return throttle_levers_;
}
}
}
