#include "LandingGear.h"

#include "../SystemPipeline.h"
#include "../SystemUpdateRates.h"

namespace
{
constexpr double kEnabledCommandThreshold = 0.5;
constexpr double kFullIntegrity = 1.0;
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
	for (std::size_t index = 0; index < wheel_radius_m_.size(); ++index)
	{
		wheel_radius_m_[index] = config.wheel_radius_m[index];
	}
	if (start_mode == StartMode::HotAir)
	{
		::Systems::configure_air_start_landing_gear(landing_gear_);
	}
	else
	{
		::Systems::configure_ground_start_landing_gear(landing_gear_);
	}
	integrity_.fill(kFullIntegrity);
	refresh_data();
}

void LandingGear::setup(SystemSetup& setup)
{
	setup.update_rate_hz(kProjectDefinedFallbackUpdateRateHz);
	setup.read(AircraftDataKeys::kFrameInput);
	setup.read(AircraftDataKeys::kAircraftObservation);
	setup.read(AircraftDataKeys::kPilotControlSignal);
	setup.publish(AircraftDataKeys::kLandingGearData, data_);
	register_handlers(setup);
}

void LandingGear::register_handlers(SystemSetup& setup)
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
			[this](const SystemActionContext&, const Command& command)
			{ handle_command(command); });
	}
	setup.register_damage_handler(
		DamageArea::LandingGear,
		[this](const SystemActionContext&, const DamageEvent& event)
		{ apply_damage(event); });
	setup.register_repair_handler(
		[this](const SystemActionContext&, const RepairEvent& event)
		{ repair(event); });
}

void LandingGear::step(
	const SystemStepContext& context,
	const AircraftDataView& aircraft,
	SystemResult& result)
{
	const FrameInput& frame = aircraft.read(AircraftDataKeys::kFrameInput);
	const AircraftObservation& observation =
		aircraft.read(AircraftDataKeys::kAircraftObservation);
	apply_suspension_feedback(frame);
	const PilotControlSignal& pilot =
		aircraft.read(AircraftDataKeys::kPilotControlSignal);
	const LandingGearFrameInput input = {
		observation.true_airspeed_mps,
		observation.ground_speed_mps,
		context.dt_s,
		observation.altitude_agl_m,
		pilot.yaw_axis_normalized
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
		landing_gear_, input.true_airspeed_mps, input.yaw_input_normalized);
	::Systems::update_nose_wheel_steering(
		landing_gear_.wheels, steering);
	::Systems::update_wheel_spin(
		landing_gear_.wheels,
		landing_gear_.position_normalized,
		{
			input.ground_speed_mps,
			input.dt_s,
			input.altitude_agl_m,
			wheel_radius_m_
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
			{ wheel.index, wheel.compression_m, wheel.acting_force_body_n });
	}
}

void LandingGear::update_on_ground()
{
	Core::Systems::update_on_ground(
		suspension_, landing_gear_.position_normalized);
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
			landing_gear_, command.value_normalized > kEnabledCommandThreshold); break;
	case CommandId::ToggleNoseWheelSteering:
		::Systems::toggle_nose_turn_enabled(
			wheels, command.value_normalized > kEnabledCommandThreshold); break;
	case CommandId::SetNoseWheelSteering:
		::Systems::set_nose_turn_enabled(
			wheels, command.value_normalized > kEnabledCommandThreshold); break;
	case CommandId::SetBrake:
		::Systems::set_brake_axis(
			wheels, ::Systems::normalize_brake_axis(command.value_normalized)); break;
	case CommandId::SetLeftBrake:
		::Systems::set_left_brake(
			wheels, ::Systems::normalize_brake_axis(command.value_normalized)); break;
	case CommandId::SetRightBrake:
		::Systems::set_right_brake(
			wheels, ::Systems::normalize_brake_axis(command.value_normalized)); break;
	default:
		break;
	}
	refresh_data();
}

void LandingGear::apply_damage(const DamageEvent& event)
{
	if (event.area != DamageArea::LandingGear ||
		event.segment >= integrity_.size())
	{
		return;
	}
	integrity_[event.segment] = event.integrity;
	refresh_data();
}

void LandingGear::repair(const RepairEvent& event)
{
	(void)event;
	integrity_.fill(kFullIntegrity);
	refresh_data();
}

void LandingGear::refresh_data()
{
	data_.position_normalized = landing_gear_.position_normalized;
	data_.handle_down = landing_gear_.switch_down;
	data_.nose_wheel_steering_normalized =
		landing_gear_.wheels.nose_steering *
		integrity_[landing_gear_segment_index(
			LandingGearDamageSegment::Nose)];
	data_.brake_left_normalized = landing_gear_.wheels.brake_left *
		integrity_[landing_gear_segment_index(
			LandingGearDamageSegment::LeftMain)];
	data_.brake_right_normalized = landing_gear_.wheels.brake_right *
		integrity_[landing_gear_segment_index(
			LandingGearDamageSegment::RightMain)];
	for (std::size_t index = 0;
		index < data_.wheel_spin_phase_0_1.size();
		++index)
	{
		data_.wheel_radius_m[index] = wheel_radius_m_[index];
		data_.wheel_spin_phase_0_1[index] =
			landing_gear_.wheels.spin[index];
		data_.suspension[index] = {
			suspension_.force_body_n[index],
			suspension_.compression_m[index],
			suspension_.force_magnitude_n[index],
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
