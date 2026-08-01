#pragma once

#include "../FlightControlReferences.h"

namespace Core
{
namespace Systems
{
struct AutopilotModeMonitorConfig
{
	double pitch_tracking_error_limit_rad = 0.0;
	double vertical_speed_tracking_error_limit_mps = 0.0;
	double bank_tracking_error_limit_rad = 0.0;
	double tracking_failure_persistence_s = 0.0;
	double actuator_saturation_persistence_s = 0.0;
};

struct AutopilotModeMonitorObservation
{
	double dt_s = 0.0;
	bool vertical_active = false;
	bool lateral_active = false;
	VerticalGuidanceReferenceType vertical_type =
		VerticalGuidanceReferenceType::None;
	double vertical_tracking_error = 0.0;
	double lateral_tracking_error_rad = 0.0;
	GuidanceConstraint constraint;
	bool actuator_saturated = false;
	ConstraintReason hard_protection_reason = ConstraintReason::None;
};

struct AutopilotModeMonitorResult
{
	ConstraintReason constraint_reason = ConstraintReason::None;
	DegradationReason degradation_reason = DegradationReason::None;
	DisconnectReason disconnect_reason = DisconnectReason::None;
	bool release_vertical = false;
	bool release_lateral = false;
};

class AutopilotModeMonitor final
{
public:
	explicit AutopilotModeMonitor(
		const AutopilotModeMonitorConfig& config);
	AutopilotModeMonitorResult update(
		const AutopilotModeMonitorObservation& observation);
	void reset_vertical();
	void reset_lateral();
	void reset_all();

private:
	bool vertical_tracking_failed(
		const AutopilotModeMonitorObservation& observation) const;
	bool lateral_tracking_failed(
		const AutopilotModeMonitorObservation& observation) const;
	static bool valid_observation(
		const AutopilotModeMonitorObservation& observation);
	static double update_timer(double timer_s, bool failed, double dt_s);

	const AutopilotModeMonitorConfig config_;
	double vertical_failure_time_s_ = 0.0;
	double lateral_failure_time_s_ = 0.0;
	double saturation_time_s_ = 0.0;
};
}
}
