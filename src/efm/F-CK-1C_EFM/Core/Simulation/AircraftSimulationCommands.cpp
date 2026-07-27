#include "AircraftSimulation.h"

namespace Core
{
namespace Simulation
{
void AircraftSimulation::handle_command(const Command& command)
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

void AircraftSimulation::handle_pitch_roll_command(const Command& command)
{
	switch (command.id)
	{
	case CommandId::SetPitchAxis:
		Systems::set_pitch_axis_input(systems_.primary_controls, command.value); break;
	case CommandId::SetPitchDiscrete:
		Systems::set_pitch_discrete_input(
			systems_.primary_controls, static_cast<int>(command.value)); break;
	case CommandId::AdjustPitchTrim:
		Systems::adjust_pitch_trim(systems_.primary_controls, command.value); break;
	case CommandId::SetRollAxis:
		Systems::set_roll_axis_input(systems_.primary_controls, command.value); break;
	case CommandId::SetRollDiscrete:
		Systems::set_roll_discrete_input(
			systems_.primary_controls, static_cast<int>(command.value)); break;
	case CommandId::AdjustRollTrim:
		Systems::adjust_roll_trim(systems_.primary_controls, command.value); break;
	default: break;
	}
}

void AircraftSimulation::handle_yaw_command(const Command& command)
{
	switch (command.id)
	{
	case CommandId::SetYawAxis:
		Systems::set_yaw_axis_input(systems_.primary_controls, command.value); break;
	case CommandId::SetYawDiscrete:
		Systems::set_yaw_discrete_input(
			systems_.primary_controls, static_cast<int>(command.value)); break;
	case CommandId::AdjustYawTrim:
		Systems::adjust_yaw_trim(systems_.primary_controls, command.value); break;
	case CommandId::ResetTrim:
		Systems::reset_primary_trims(systems_.primary_controls); break;
	default: break;
	}
}

void AircraftSimulation::handle_fbw_command(const Command& command)
{
	const bool enabled = command.value > 0.5;
	switch (command.id)
	{
	case CommandId::ToggleFbwCat:
		Systems::toggle_fbw_cat_mode(systems_.fbw, enabled); break;
	case CommandId::SetFbwCat1:
		if (enabled) Systems::set_fbw_cat_mode(systems_.fbw, Systems::FBW_CAT1); break;
	case CommandId::SetFbwCat3:
		if (enabled) Systems::set_fbw_cat_mode(systems_.fbw, Systems::FBW_CAT3); break;
	case CommandId::SetGLimiterOverride:
		Systems::set_fbw_g_limiter_override(systems_.fbw, enabled); break;
	case CommandId::ToggleGLimiterOverride:
		Systems::toggle_fbw_g_limiter_override(systems_.fbw, enabled); break;
	default: break;
	}
}

void AircraftSimulation::handle_engine_command(const Command& command)
{
	const bool enabled = command.value > 0.5;
	switch (command.id)
	{
	case CommandId::SetBothEngines:
		Systems::set_both_engine_switches(systems_.engines, enabled); break;
	case CommandId::SetLeftEngine:
		Systems::set_left_engine_switch(systems_.engines, enabled); break;
	case CommandId::SetRightEngine:
		Systems::set_right_engine_switch(systems_.engines, enabled); break;
	default: break;
	}
}

void AircraftSimulation::handle_throttle_command(const Command& command)
{
	switch (command.id)
	{
	case CommandId::SetCommonThrottleAxis:
		Systems::set_common_throttle_axis(systems_.throttle_inputs, command.value); break;
	case CommandId::SetLeftThrottleAxis:
		Systems::set_left_throttle_axis(systems_.throttle_inputs, command.value); break;
	case CommandId::SetRightThrottleAxis:
		Systems::set_right_throttle_axis(systems_.throttle_inputs, command.value); break;
	case CommandId::StepCommonThrottle:
		Systems::step_common_keyboard_throttle(systems_.throttle_inputs, command.value); break;
	case CommandId::StepLeftThrottle:
		Systems::step_left_keyboard_throttle(systems_.throttle_inputs, command.value); break;
	case CommandId::StepRightThrottle:
		Systems::step_right_keyboard_throttle(systems_.throttle_inputs, command.value); break;
	default: break;
	}
}

void AircraftSimulation::handle_airframe_command(const Command& command)
{
	switch (command.id)
	{
	case CommandId::ToggleAirbrake:
		Systems::toggle_airbrake(systems_.airframe_devices); break;
	case CommandId::SetAirbrake:
		Systems::set_airbrake(systems_.airframe_devices, command.value > 0.5); break;
	case CommandId::ToggleFlaps:
		Systems::toggle_flap_mode(systems_.airframe_devices); break;
	case CommandId::SetFlapsUp:
		Systems::set_flap_mode(systems_.airframe_devices, Systems::FLAP_MODE_UP); break;
	case CommandId::SetFlapsAuto:
		Systems::set_flap_mode(systems_.airframe_devices, Systems::FLAP_MODE_AUTO); break;
	case CommandId::SetFlapsDown:
		Systems::set_flap_mode(systems_.airframe_devices, Systems::FLAP_MODE_DOWN); break;
	default: break;
	}
}

void AircraftSimulation::handle_landing_gear_command(const Command& command)
{
	Systems::WheelState& wheels = systems_.landing_gear.wheels;
	switch (command.id)
	{
	case CommandId::ToggleGear:
		Systems::toggle_gear(systems_.landing_gear); break;
	case CommandId::SetGear:
		Systems::set_gear(systems_.landing_gear, command.value > 0.5); break;
	case CommandId::ToggleNoseWheelSteering:
		Systems::toggle_nose_turn_enabled(wheels, command.value > 0.5); break;
	case CommandId::SetNoseWheelSteering:
		Systems::set_nose_turn_enabled(wheels, command.value > 0.5); break;
	case CommandId::SetBrake:
		Systems::set_brake_axis(wheels, Systems::normalize_brake_axis(command.value)); break;
	case CommandId::SetLeftBrake:
		Systems::set_left_brake(wheels, Systems::normalize_brake_axis(command.value)); break;
	case CommandId::SetRightBrake:
		Systems::set_right_brake(wheels, Systems::normalize_brake_axis(command.value)); break;
	default: break;
	}
}
}
}
