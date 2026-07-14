#include "Fck1cEfm.h"

namespace Core
{
void Fck1cEfm::handle_command(const EfmCommand& command)
{
	switch (command.group)
	{
	case CommandGroup::PitchRoll: handle_pitch_roll_command(command); break;
	case CommandGroup::Yaw: handle_yaw_command(command); break;
	case CommandGroup::Fbw: handle_fbw_command(command); break;
	case CommandGroup::Engine: handle_engine_command(command); break;
	case CommandGroup::Throttle: handle_throttle_command(command); break;
	case CommandGroup::Airframe: handle_airframe_command(command); break;
	case CommandGroup::LandingGear: handle_landing_gear_command(command); break;
	case CommandGroup::None: break;
	}
}

void Fck1cEfm::handle_pitch_roll_command(const EfmCommand& command)
{
	switch (command.action)
	{
	case CommandAction::SetPitchAxis:
		Systems::set_pitch_axis_input(systems_.primary_controls, command.value); break;
	case CommandAction::SetPitchDiscrete:
		Systems::set_pitch_discrete_input(
			systems_.primary_controls, static_cast<int>(command.value)); break;
	case CommandAction::AdjustPitchTrim:
		Systems::adjust_pitch_trim(systems_.primary_controls, command.value); break;
	case CommandAction::SetRollAxis:
		Systems::set_roll_axis_input(systems_.primary_controls, command.value); break;
	case CommandAction::SetRollDiscrete:
		Systems::set_roll_discrete_input(
			systems_.primary_controls, static_cast<int>(command.value)); break;
	case CommandAction::AdjustRollTrim:
		Systems::adjust_roll_trim(systems_.primary_controls, command.value); break;
	default: break;
	}
}

void Fck1cEfm::handle_yaw_command(const EfmCommand& command)
{
	switch (command.action)
	{
	case CommandAction::SetYawAxis:
		Systems::set_yaw_axis_input(systems_.primary_controls, command.value); break;
	case CommandAction::SetYawDiscrete:
		Systems::set_yaw_discrete_input(
			systems_.primary_controls, static_cast<int>(command.value)); break;
	case CommandAction::AdjustYawTrim:
		Systems::adjust_yaw_trim(systems_.primary_controls, command.value); break;
	case CommandAction::ResetTrim:
		Systems::reset_primary_trims(systems_.primary_controls); break;
	default: break;
	}
}

void Fck1cEfm::handle_fbw_command(const EfmCommand& command)
{
	const bool enabled = command.value > 0.5;
	switch (command.action)
	{
	case CommandAction::ToggleFbwCat:
		Systems::toggle_fbw_cat_mode(systems_.fbw, enabled); break;
	case CommandAction::SetFbwCat1:
		if (enabled) Systems::set_fbw_cat_mode(systems_.fbw, Systems::FBW_CAT1); break;
	case CommandAction::SetFbwCat3:
		if (enabled) Systems::set_fbw_cat_mode(systems_.fbw, Systems::FBW_CAT3); break;
	case CommandAction::SetGLimiterOverride:
		Systems::set_fbw_g_limiter_override(systems_.fbw, enabled); break;
	case CommandAction::ToggleGLimiterOverride:
		Systems::toggle_fbw_g_limiter_override(systems_.fbw, enabled); break;
	default: break;
	}
}

void Fck1cEfm::handle_engine_command(const EfmCommand& command)
{
	const bool enabled = command.value > 0.5;
	switch (command.action)
	{
	case CommandAction::SetBothEngines:
		Systems::set_both_engine_switches(systems_.engines, enabled); break;
	case CommandAction::SetLeftEngine:
		Systems::set_left_engine_switch(systems_.engines, enabled); break;
	case CommandAction::SetRightEngine:
		Systems::set_right_engine_switch(systems_.engines, enabled); break;
	default: break;
	}
}

void Fck1cEfm::handle_throttle_command(const EfmCommand& command)
{
	switch (command.action)
	{
	case CommandAction::SetCommonThrottleAxis:
		Systems::set_common_throttle_axis(systems_.throttle_inputs, command.value); break;
	case CommandAction::SetLeftThrottleAxis:
		Systems::set_left_throttle_axis(systems_.throttle_inputs, command.value); break;
	case CommandAction::SetRightThrottleAxis:
		Systems::set_right_throttle_axis(systems_.throttle_inputs, command.value); break;
	case CommandAction::StepCommonThrottle:
		Systems::step_common_keyboard_throttle(systems_.throttle_inputs, command.value); break;
	case CommandAction::StepLeftThrottle:
		Systems::step_left_keyboard_throttle(systems_.throttle_inputs, command.value); break;
	case CommandAction::StepRightThrottle:
		Systems::step_right_keyboard_throttle(systems_.throttle_inputs, command.value); break;
	default: break;
	}
}

void Fck1cEfm::handle_airframe_command(const EfmCommand& command)
{
	switch (command.action)
	{
	case CommandAction::ToggleAirbrake:
		Systems::toggle_airbrake(systems_.airframe_devices); break;
	case CommandAction::SetAirbrake:
		Systems::set_airbrake(systems_.airframe_devices, command.value > 0.5); break;
	case CommandAction::ToggleFlaps:
		Systems::toggle_flap_mode(systems_.airframe_devices); break;
	case CommandAction::SetFlapsUp:
		Systems::set_flap_mode(systems_.airframe_devices, Systems::FLAP_MODE_UP); break;
	case CommandAction::SetFlapsAuto:
		Systems::set_flap_mode(systems_.airframe_devices, Systems::FLAP_MODE_AUTO); break;
	case CommandAction::SetFlapsDown:
		Systems::set_flap_mode(systems_.airframe_devices, Systems::FLAP_MODE_DOWN); break;
	default: break;
	}
}

void Fck1cEfm::handle_landing_gear_command(const EfmCommand& command)
{
	Systems::WheelState& wheels = systems_.landing_gear.wheels;
	switch (command.action)
	{
	case CommandAction::ToggleGear:
		Systems::toggle_gear(systems_.landing_gear); break;
	case CommandAction::SetGear:
		Systems::set_gear(systems_.landing_gear, command.value > 0.5); break;
	case CommandAction::ToggleNoseWheelSteering:
		Systems::toggle_nose_turn_enabled(wheels, command.value > 0.5); break;
	case CommandAction::SetNoseWheelSteering:
		Systems::set_nose_turn_enabled(wheels, command.value > 0.5); break;
	case CommandAction::SetBrake:
		Systems::set_brake_axis(wheels, Systems::normalize_brake_axis(command.value)); break;
	case CommandAction::SetLeftBrake:
		Systems::set_left_brake(wheels, Systems::normalize_brake_axis(command.value)); break;
	case CommandAction::SetRightBrake:
		Systems::set_right_brake(wheels, Systems::normalize_brake_axis(command.value)); break;
	default: break;
	}
}
}
