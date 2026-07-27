#include "LandingGear.h"

#include "../SystemPipeline.h"

namespace
{
constexpr double kEnabledCommandThreshold = 0.5;
}

namespace Core
{
namespace Systems
{
LandingGear::LandingGear(
	StartMode start_mode,
	const LandingGearConfig& config)
{
	validate_landing_gear_config(config);
	for (std::size_t index = 0; index < wheel_radius_.size(); ++index)
	{
		wheel_radius_[index] = config.wheel_radius[index];
	}
	if (start_mode == StartMode::HotAir)
	{
		::Systems::configure_air_start_landing_gear(landing_gear_);
	}
	else
	{
		::Systems::configure_ground_start_landing_gear(landing_gear_);
	}
	refresh_data();
}

void LandingGear::setup(SystemSetup& setup)
{
	setup.read(AircraftDataKeys::kFrameInput);
	setup.read(AircraftDataKeys::kAircraftObservation);
	setup.read(AircraftDataKeys::kPilotControlState);
	setup.publish(AircraftDataKeys::kLandingGearData, data_);
	register_commands(setup);
}

void LandingGear::register_commands(SystemSetup& setup)
{
	const CommandId commands[] = {
		CommandId::ToggleGear,
		CommandId::SetGear,
		CommandId::ToggleNoseWheelSteering,
		CommandId::SetNoseWheelSteering,
		CommandId::SetBrake,
		CommandId::SetLeftBrake,
		CommandId::SetRightBrake
	};
	for (CommandId id : commands)
	{
		setup.register_command_handler(
			id,
			[this](const Command& command) { handle_command(command); });
	}
}

void LandingGear::step(
	const AircraftDataSnapshot& snapshot,
	SystemResult& result)
{
	const FrameInput& frame = snapshot.read(AircraftDataKeys::kFrameInput);
	const AircraftObservation& observation =
		snapshot.read(AircraftDataKeys::kAircraftObservation);
	apply_suspension_feedback(frame);
	const PilotControlState& pilot =
		snapshot.read(AircraftDataKeys::kPilotControlState);
	const LandingGearFrameInput input = {
		observation.speed_scalar,
		observation.ground_speed,
		frame.dt_s,
		observation.altitude_agl,
		pilot.yaw
	};
	step(input);
	update_on_ground();
	result.publish(AircraftDataKeys::kLandingGearData, data_);
}

const LandingGearData& LandingGear::step(
	const LandingGearFrameInput& input)
{
	::Systems::update_gear_position(landing_gear_);
	const double steering = ::Systems::compute_nose_wheel_steering(
		landing_gear_, input.speed_scalar, input.yaw_input);
	::Systems::update_nose_wheel_steering(
		landing_gear_.wheels, steering);
	::Systems::update_wheel_spin(
		landing_gear_.wheels,
		landing_gear_.position,
		{
			input.ground_speed,
			input.dt,
			input.altitude_agl,
			wheel_radius_
		});
	refresh_data();
	return data_;
}

void LandingGear::apply_suspension_feedback(const FrameInput& input)
{
	for (std::size_t index = 0; index < input.suspension.size(); ++index)
	{
		if (!input.availability.suspension[index])
		{
			continue;
		}
		const SuspensionFeedbackInput& wheel = input.suspension[index];
		update_suspension_feedback(
			suspension_,
			{ wheel.index, wheel.compression, wheel.acting_force });
	}
}

void LandingGear::update_on_ground()
{
	Core::Systems::update_on_ground(suspension_, landing_gear_.position);
	refresh_data();
}

void LandingGear::handle_command(const Command& command)
{
	::Systems::WheelState& wheels = landing_gear_.wheels;
	switch (command.id)
	{
	case CommandId::ToggleGear:
		::Systems::toggle_gear(landing_gear_); break;
	case CommandId::SetGear:
		::Systems::set_gear(
			landing_gear_, command.value > kEnabledCommandThreshold); break;
	case CommandId::ToggleNoseWheelSteering:
		::Systems::toggle_nose_turn_enabled(
			wheels, command.value > kEnabledCommandThreshold); break;
	case CommandId::SetNoseWheelSteering:
		::Systems::set_nose_turn_enabled(
			wheels, command.value > kEnabledCommandThreshold); break;
	case CommandId::SetBrake:
		::Systems::set_brake_axis(
			wheels, ::Systems::normalize_brake_axis(command.value)); break;
	case CommandId::SetLeftBrake:
		::Systems::set_left_brake(
			wheels, ::Systems::normalize_brake_axis(command.value)); break;
	case CommandId::SetRightBrake:
		::Systems::set_right_brake(
			wheels, ::Systems::normalize_brake_axis(command.value)); break;
	default:
		break;
	}
	refresh_data();
}

void LandingGear::refresh_data()
{
	data_.position = landing_gear_.position;
	data_.nose_wheel_steering = landing_gear_.wheels.nose_steering;
	data_.brake_left = landing_gear_.wheels.brake_left;
	data_.brake_right = landing_gear_.wheels.brake_right;
	for (std::size_t index = 0; index < data_.wheel_spin.size(); ++index)
	{
		data_.wheel_radius[index] = wheel_radius_[index];
		data_.wheel_spin[index] = landing_gear_.wheels.spin[index];
		data_.suspension[index] = {
			suspension_.force[index],
			suspension_.compression[index],
			suspension_.force_magnitude[index],
			suspension_.weight_on_wheel[index]
		};
	}
	data_.any_weight_on_wheels = any_weight_on_wheels(suspension_);
	data_.on_ground = suspension_.on_ground;
}

const LandingGearData& LandingGear::data() const
{
	return data_;
}

const ::Systems::LandingGearSystemState& LandingGear::device_state() const
{
	return landing_gear_;
}

const SuspensionFeedbackState& LandingGear::suspension_state() const
{
	return suspension_;
}

}
}
