#include "FlightControlComputer.h"

#include "../SystemPipeline.h"
#include "Common/Table.h"

#include <stdexcept>

namespace
{
constexpr double kColdStartThrottle = 0.0;
constexpr double kHotAirStartThrottle = 0.5;
constexpr double kEnabledCommandThreshold = 0.5;
}

namespace Core
{
namespace Systems
{
FlightControlComputer::FlightControlComputer(
	const FlightControlComputerConfig& config,
	StartMode start_mode)
	: config_(config)
{
	validate_flight_control_computer_config(config_);
	const double throttle = start_mode == StartMode::HotAir
		? kHotAirStartThrottle : kColdStartThrottle;
	::Systems::reset_throttle_inputs(
		throttle_inputs_, throttle, throttle);
	::Systems::reset_fbw_state(fbw_, {});
	refresh_outputs({});
}

void FlightControlComputer::setup(SystemSetup& setup)
{
	setup.read(AircraftDataKeys::kFrameInput);
	setup.read(AircraftDataKeys::kAircraftObservation);
	setup.read(AircraftDataKeys::kLandingGearData);
	setup.read(AircraftDataKeys::kPrimaryControlPosition);
	setup.publish(AircraftDataKeys::kPilotControlState, pilot_controls_);
	setup.publish(AircraftDataKeys::kFlightControlDemand, demand_);
	setup.publish(AircraftDataKeys::kEngineControlDemand, engine_demand_);
	register_commands(setup);
}

void FlightControlComputer::register_commands(SystemSetup& setup)
{
	const CommandId commands[] = {
		CommandId::SetPitchAxis, CommandId::SetPitchDiscrete,
		CommandId::AdjustPitchTrim, CommandId::SetRollAxis,
		CommandId::SetRollDiscrete, CommandId::AdjustRollTrim,
		CommandId::SetYawAxis, CommandId::SetYawDiscrete,
		CommandId::AdjustYawTrim, CommandId::ResetTrim,
		CommandId::ToggleFbwCat, CommandId::SetFbwCat1,
		CommandId::SetFbwCat3, CommandId::SetGLimiterOverride,
		CommandId::ToggleGLimiterOverride,
		CommandId::SetCommonThrottleAxis, CommandId::SetLeftThrottleAxis,
		CommandId::SetRightThrottleAxis, CommandId::StepCommonThrottle,
		CommandId::StepLeftThrottle, CommandId::StepRightThrottle
	};
	for (CommandId id : commands)
	{
		setup.register_command_handler(
			id,
			[this](const Command& command) { handle_command(command); });
	}
}

void FlightControlComputer::step(
	const AircraftDataView& aircraft,
	SystemResult& result)
{
	const FrameInput& frame = aircraft.read(AircraftDataKeys::kFrameInput);
	step(make_pipeline_input(aircraft), frame.autopilot);
	result.publish(AircraftDataKeys::kPilotControlState, pilot_controls_);
	result.publish(AircraftDataKeys::kFlightControlDemand, demand_);
	result.publish(AircraftDataKeys::kEngineControlDemand, engine_demand_);
}

const FlightControlDemand& FlightControlComputer::step(
	::Systems::FBWControllerInput input,
	const AutopilotCommand& autopilot)
{
	input.alpha_limit_deg = alpha_limit(input.mach);
	::Systems::update_primary_control_inputs(primary_controls_);
	apply_autopilot(autopilot);
	input.roll_input = primary_controls_.roll.input;
	input.roll_trim = primary_controls_.roll.trim;
	input.pitch_input = primary_controls_.pitch.input;
	input.pitch_trim = primary_controls_.pitch.trim;
	input.yaw_input = primary_controls_.yaw.input;
	input.yaw_trim = primary_controls_.yaw.trim;
	const ::Systems::FBWControllerOutput output =
		::Systems::update_fbw_controller(
			fbw_, config_.control_laws, input);
	::Systems::update_pilot_throttle_cmds(throttle_inputs_);
	refresh_outputs(output);
	return demand_;
}

void FlightControlComputer::apply_autopilot(
	const AutopilotCommand& autopilot)
{
	if (autopilot.master && !autopilot.bypass)
	{
		primary_controls_.pitch.input = autopilot.pitch_command;
		primary_controls_.roll.input = autopilot.roll_command;
	}
	if (autopilot.auto_throttle_engaged)
	{
		fbw_.throttle_cmd_left = autopilot.throttle_command;
		fbw_.throttle_cmd_right = autopilot.throttle_command;
		fbw_.throttle_blend = 1.0;
		fbw_.throttle_override = false;
		return;
	}
	fbw_.throttle_blend = 0.0;
}

void FlightControlComputer::refresh_outputs(
	const ::Systems::FBWControllerOutput& output)
{
	refresh_pilot_controls();
	demand_ = {
		output.elevator_command,
		output.aileron_command,
		output.rudder_command
	};
	engine_demand_ = {
		::Systems::compose_engine_throttle_cmd({
			throttle_inputs_.left.pilot_cmd,
			fbw_.throttle_cmd_left,
			fbw_.throttle_blend,
			fbw_.throttle_override
		}),
		::Systems::compose_engine_throttle_cmd({
			throttle_inputs_.right.pilot_cmd,
			fbw_.throttle_cmd_right,
			fbw_.throttle_blend,
			fbw_.throttle_override
		})
	};
}

void FlightControlComputer::refresh_pilot_controls()
{
	pilot_controls_ = {
		primary_controls_.pitch.input,
		primary_controls_.roll.input,
		primary_controls_.yaw.input
	};
}

::Systems::FBWControllerInput FlightControlComputer::make_pipeline_input(
	const AircraftDataView& aircraft) const
{
	const FrameInput& frame = aircraft.read(AircraftDataKeys::kFrameInput);
	const AircraftObservation& observation =
		aircraft.read(AircraftDataKeys::kAircraftObservation);
	::Systems::FBWControllerInput input;
	input.dt = frame.dt_s;
	input.qbar = observation.dynamic_pressure;
	input.roll = observation.roll;
	input.pitch = observation.pitch;
	input.roll_rate = observation.roll_rate;
	input.pitch_rate = observation.pitch_rate;
	input.yaw_rate = observation.yaw_rate;
	input.alpha = observation.alpha_deg;
	input.beta = observation.beta_deg;
	input.speed_scalar = observation.speed_scalar;
	input.mach = observation.mach;
	input.g = observation.g_load;
	const LandingGearData& gear =
		aircraft.read(AircraftDataKeys::kLandingGearData);
	input.gear_pos = gear.position;
	input.wow = gear.any_weight_on_wheels;
	const PrimaryControlPosition& position =
		aircraft.read(AircraftDataKeys::kPrimaryControlPosition);
	input.elevator_command = position.elevator;
	input.aileron_command = position.aileron;
	input.rudder_command = position.rudder;
	return input;
}

double FlightControlComputer::alpha_limit(double mach) const
{
	return Common::lerp(
		{
			config_.mach_table.data(),
			config_.alpha_limit_deg.data(),
			static_cast<unsigned>(config_.mach_table.size())
		},
		mach);
}

void FlightControlComputer::handle_command(const Command& command)
{
	handle_primary_command(command);
	handle_yaw_command(command);
	handle_fbw_command(command);
	handle_throttle_command(command);
	refresh_pilot_controls();
}

void FlightControlComputer::handle_primary_command(const Command& command)
{
	switch (command.id)
	{
	case CommandId::SetPitchAxis:
		::Systems::set_pitch_axis_input(primary_controls_, command.value); break;
	case CommandId::SetPitchDiscrete:
		::Systems::set_pitch_discrete_input(
			primary_controls_, static_cast<int>(command.value)); break;
	case CommandId::AdjustPitchTrim:
		::Systems::adjust_pitch_trim(primary_controls_, command.value); break;
	case CommandId::SetRollAxis:
		::Systems::set_roll_axis_input(primary_controls_, command.value); break;
	case CommandId::SetRollDiscrete:
		::Systems::set_roll_discrete_input(
			primary_controls_, static_cast<int>(command.value)); break;
	case CommandId::AdjustRollTrim:
		::Systems::adjust_roll_trim(primary_controls_, command.value); break;
	default:
		break;
	}
}

void FlightControlComputer::handle_yaw_command(const Command& command)
{
	switch (command.id)
	{
	case CommandId::SetYawAxis:
		::Systems::set_yaw_axis_input(primary_controls_, command.value); break;
	case CommandId::SetYawDiscrete:
		::Systems::set_yaw_discrete_input(
			primary_controls_, static_cast<int>(command.value)); break;
	case CommandId::AdjustYawTrim:
		::Systems::adjust_yaw_trim(primary_controls_, command.value); break;
	case CommandId::ResetTrim:
		::Systems::reset_primary_trims(primary_controls_); break;
	default:
		break;
	}
}

void FlightControlComputer::handle_fbw_command(const Command& command)
{
	const bool enabled = command.value > kEnabledCommandThreshold;
	switch (command.id)
	{
	case CommandId::ToggleFbwCat:
		::Systems::toggle_fbw_cat_mode(fbw_, enabled); break;
	case CommandId::SetFbwCat1:
		if (enabled) ::Systems::set_fbw_cat_mode(fbw_, ::Systems::FBW_CAT1);
		break;
	case CommandId::SetFbwCat3:
		if (enabled) ::Systems::set_fbw_cat_mode(fbw_, ::Systems::FBW_CAT3);
		break;
	case CommandId::SetGLimiterOverride:
		::Systems::set_fbw_g_limiter_override(fbw_, enabled); break;
	case CommandId::ToggleGLimiterOverride:
		::Systems::toggle_fbw_g_limiter_override(fbw_, enabled); break;
	default:
		break;
	}
}

void FlightControlComputer::handle_throttle_command(const Command& command)
{
	switch (command.id)
	{
	case CommandId::SetCommonThrottleAxis:
		::Systems::set_common_throttle_axis(
			throttle_inputs_, command.value); break;
	case CommandId::SetLeftThrottleAxis:
		::Systems::set_left_throttle_axis(
			throttle_inputs_, command.value); break;
	case CommandId::SetRightThrottleAxis:
		::Systems::set_right_throttle_axis(
			throttle_inputs_, command.value); break;
	case CommandId::StepCommonThrottle:
		::Systems::step_common_keyboard_throttle(
			throttle_inputs_, command.value); break;
	case CommandId::StepLeftThrottle:
		::Systems::step_left_keyboard_throttle(
			throttle_inputs_, command.value); break;
	case CommandId::StepRightThrottle:
		::Systems::step_right_keyboard_throttle(
			throttle_inputs_, command.value); break;
	default:
		break;
	}
}

const PilotControlState& FlightControlComputer::pilot_controls() const
{
	return pilot_controls_;
}

const FlightControlDemand& FlightControlComputer::demand() const
{
	return demand_;
}

const EngineControlDemand& FlightControlComputer::engine_demand() const
{
	return engine_demand_;
}
}
}
