#include "ExperimentalAutoThrottleAssist.h"

#include "Common/Clamp.h"

namespace
{
constexpr double kEnabledCommandThreshold = 0.5;
constexpr double kMinimumThrottleCommand = 0.0;
constexpr double kMaximumNormalizedCommand = 1.0;
constexpr double kSpeedIntegralLimit = 30.0;

bool pressed(const Core::Command& command)
{
	return command.value > kEnabledCommandThreshold;
}
}

namespace Core
{
namespace Systems
{
ExperimentalAutoThrottleAssist::ExperimentalAutoThrottleAssist(
	const AutomaticFlightControlConfig& config)
	: config_(config)
{
}

bool ExperimentalAutoThrottleAssist::handle_command(
	const Command& command,
	const AutomaticFlightControlObservation& observation)
{
	if (!pressed(command)) return false;
	switch (command.id)
	{
	case CommandId::ToggleAutoThrottle:
		if (result_.engaged)
			disengage(AutomaticFlightControlReason::Commanded);
		else
			engage(observation);
		return true;
	case CommandId::EngageAutoThrottle:
		if (!result_.engaged) engage(observation);
		return true;
	case CommandId::DisengageAutoThrottle:
		disengage(AutomaticFlightControlReason::Commanded);
		return true;
	case CommandId::IncreaseAutopilotSpeed:
		if (result_.engaged) adjust_speed_reference(1.0);
		return true;
	case CommandId::DecreaseAutopilotSpeed:
		if (result_.engaged) adjust_speed_reference(-1.0);
		return true;
	default:
		return false;
	}
}

const ExperimentalAutoThrottleResult& ExperimentalAutoThrottleAssist::update(
	const AutomaticFlightControlObservation& observation)
{
	if (!result_.engaged)
	{
		result_.throttle_normalized = 0.0;
		return result_;
	}
	if (observation.mach > config_.auto_throttle_disconnect_mach)
	{
		disengage(AutomaticFlightControlReason::MachLimit);
		return result_;
	}
	if (observation.mach > config_.maximum_auto_throttle_mach)
	{
		result_.throttle_normalized = Common::limit(
			result_.throttle_normalized -
				config_.mach_guard_throttle_rate_per_s * observation.dt_s,
			kMinimumThrottleCommand,
			kMaximumNormalizedCommand);
		return result_;
	}
	const double error =
		result_.target_speed_mps - observation.indicated_airspeed_mps;
	speed_integral_ = Common::limit(
		speed_integral_ + error * observation.dt_s,
		-kSpeedIntegralLimit,
		kSpeedIntegralLimit);
	const double command = config_.auto_throttle_base +
		config_.auto_throttle_kp * error +
		config_.auto_throttle_ki * speed_integral_;
	result_.throttle_normalized = Common::limit(
		command,
		kMinimumThrottleCommand,
		config_.auto_throttle_command_limit);
	return result_;
}

void ExperimentalAutoThrottleAssist::disengage(
	AutomaticFlightControlReason reason)
{
	if (result_.engaged || reason == AutomaticFlightControlReason::Commanded)
	{
		result_.disengage_reason = reason;
	}
	reset_controller();
}

const ExperimentalAutoThrottleResult&
ExperimentalAutoThrottleAssist::result() const
{
	return result_;
}

bool ExperimentalAutoThrottleAssist::can_engage(
	const AutomaticFlightControlObservation& observation)
{
	result_.engage_rejection_reason = AutomaticFlightControlReason::None;
	if (observation.mach > config_.maximum_auto_throttle_mach)
	{
		result_.engage_rejection_reason = AutomaticFlightControlReason::MachLimit;
	}
	else if (observation.weight_on_wheels)
	{
		result_.engage_rejection_reason =
			AutomaticFlightControlReason::WeightOnWheels;
	}
	return result_.engage_rejection_reason == AutomaticFlightControlReason::None;
}

void ExperimentalAutoThrottleAssist::engage(
	const AutomaticFlightControlObservation& observation)
{
	if (!can_engage(observation)) return;
	result_.engaged = true;
	result_.disengage_reason = AutomaticFlightControlReason::None;
	result_.target_speed_mps = observation.indicated_airspeed_mps;
	speed_integral_ = 0.0;
}

void ExperimentalAutoThrottleAssist::adjust_speed_reference(double direction)
{
	result_.target_speed_mps = Common::limit(
		result_.target_speed_mps + direction * config_.speed_step_mps,
		config_.minimum_target_speed_mps,
		config_.maximum_target_speed_mps);
}

void ExperimentalAutoThrottleAssist::reset_controller()
{
	result_.engaged = false;
	result_.throttle_normalized = 0.0;
	speed_integral_ = 0.0;
}
}
}
