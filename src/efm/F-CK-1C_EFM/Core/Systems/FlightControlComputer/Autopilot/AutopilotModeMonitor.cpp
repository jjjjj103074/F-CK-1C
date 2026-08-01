#include "AutopilotModeMonitor.h"

#include <cmath>
#include <stdexcept>

namespace Core
{
namespace Systems
{
AutopilotModeMonitor::AutopilotModeMonitor(
	const AutopilotModeMonitorConfig& config)
	: config_(config)
{
	const bool valid = config_.pitch_tracking_error_limit_rad > 0.0 &&
		config_.vertical_speed_tracking_error_limit_mps > 0.0 &&
		config_.bank_tracking_error_limit_rad > 0.0 &&
		config_.tracking_failure_persistence_s > 0.0 &&
		config_.actuator_saturation_persistence_s > 0.0;
	if (!valid)
	{
		throw std::invalid_argument("Invalid autopilot mode monitor config.");
	}
}

AutopilotModeMonitorResult AutopilotModeMonitor::update(
	const AutopilotModeMonitorObservation& observation)
{
	if (!valid_observation(observation))
	{
		AutopilotModeMonitorResult invalid;
		invalid.disconnect_reason = DisconnectReason::InvalidInput;
		return invalid;
	}
	update_failure_timers(observation);
	return make_result(observation);
}

void AutopilotModeMonitor::update_failure_timers(
	const AutopilotModeMonitorObservation& observation)
{
	vertical_failure_time_s_ = update_timer(
		vertical_failure_time_s_,
		observation.vertical_active &&
			(vertical_tracking_failed(observation) ||
				observation.constraint.vertical_constrained),
		observation.dt_s);
	lateral_failure_time_s_ = update_timer(
		lateral_failure_time_s_,
		observation.lateral_active &&
			(lateral_tracking_failed(observation) ||
				observation.constraint.lateral_constrained),
		observation.dt_s);
	saturation_time_s_ = update_timer(
		saturation_time_s_,
		observation.actuator_saturated &&
			(observation.vertical_active || observation.lateral_active),
		observation.dt_s);
}

AutopilotModeMonitorResult AutopilotModeMonitor::make_result(
	const AutopilotModeMonitorObservation& observation) const
{
	AutopilotModeMonitorResult result;
	result.vertical_constrained = observation.vertical_active &&
		observation.constraint.vertical_constrained;
	result.lateral_constrained = observation.lateral_active &&
		observation.constraint.lateral_constrained;
	result.constraint_reason = observation.constraint.reason !=
		ConstraintReason::None
		? observation.constraint.reason
		: observation.hard_protection_reason;
	if (vertical_failure_time_s_ >= config_.tracking_failure_persistence_s)
	{
		result.degradation_reason =
			DegradationReason::SustainedVerticalTrackingFailure;
		result.release_vertical = true;
	}
	else if (lateral_failure_time_s_ >= config_.tracking_failure_persistence_s)
	{
		result.degradation_reason =
			DegradationReason::SustainedLateralTrackingFailure;
		result.release_lateral = true;
	}
	else if (saturation_time_s_ >=
		config_.actuator_saturation_persistence_s)
	{
		result.degradation_reason =
			DegradationReason::SustainedActuatorSaturation;
		result.release_vertical = observation.vertical_active;
		result.release_lateral = observation.lateral_active;
	}
	return result;
}

bool AutopilotModeMonitor::valid_observation(
	const AutopilotModeMonitorObservation& observation)
{
	return std::isfinite(observation.dt_s) && observation.dt_s > 0.0 &&
		std::isfinite(observation.vertical_tracking_error) &&
		std::isfinite(observation.lateral_tracking_error_rad);
}

bool AutopilotModeMonitor::vertical_tracking_failed(
	const AutopilotModeMonitorObservation& observation) const
{
	if (!observation.vertical_active) return false;
	const double limit = observation.vertical_type ==
		VerticalGuidanceReferenceType::PitchAttitude
		? config_.pitch_tracking_error_limit_rad
		: config_.vertical_speed_tracking_error_limit_mps;
	return std::fabs(observation.vertical_tracking_error) > limit;
}

bool AutopilotModeMonitor::lateral_tracking_failed(
	const AutopilotModeMonitorObservation& observation) const
{
	return observation.lateral_active &&
		std::fabs(observation.lateral_tracking_error_rad) >
			config_.bank_tracking_error_limit_rad;
}

double AutopilotModeMonitor::update_timer(
	double timer_s,
	bool failed,
	double dt_s)
{
	return failed ? timer_s + dt_s : 0.0;
}

void AutopilotModeMonitor::reset_vertical()
{
	vertical_failure_time_s_ = 0.0;
}

void AutopilotModeMonitor::reset_lateral()
{
	lateral_failure_time_s_ = 0.0;
}

void AutopilotModeMonitor::reset_all()
{
	reset_vertical();
	reset_lateral();
	reset_saturation();
}

void AutopilotModeMonitor::reset_saturation()
{
	saturation_time_s_ = 0.0;
}
}
}
