#pragma once

#include "ControlLaws/ControlLawConfig.h"
#include "ControlLaws/ControlLawSignals.h"
#include "../../Contracts/AircraftData.h"

namespace Core
{
namespace Systems
{
struct RawFlightControlInput
{
	double dt_s = 0.0;
	double alpha_limit_deg = 0.0;
	FlightControlObservation observation;
	PilotControlSignal pilot;
	LandingGearData landing_gear;
	FlightControlActuatorState actuator;
};

class InputSignalManagement
{
public:
	explicit InputSignalManagement(
		const ::Systems::FBWControllerConfig& config);

	::Systems::ConditionedFlightControlInput condition(
		const RawFlightControlInput& raw,
		::Systems::FBWCatMode cat_mode);
	double cat_mode_blend() const;

private:
	double filter(double current, double target, double tau_s, double dt_s) const;
	double filter_angle(
		double current_rad,
		double target_rad,
		double tau_s,
		double dt_s) const;
	double shape_stick(double value, double exponent_weight) const;
	void validate(const RawFlightControlInput& raw) const;
	void update_observation(
		::Systems::ConditionedFlightControlInput& output,
		const RawFlightControlInput& raw);
	void update_pilot_signal(
		::Systems::ConditionedFlightControlInput& output,
		const PilotControlSignal& pilot,
		const ::Systems::FBWCatParams& cat);
	void update_actuator_feedback(
		::Systems::ConditionedFlightControlInput& output,
		const RawFlightControlInput& raw) const;

	const ::Systems::FBWControllerConfig config_;
	::Systems::ConditionedFlightControlInput state_;
	double shaped_roll_normalized_ = 0.0;
	double shaped_pitch_normalized_ = 0.0;
	double shaped_yaw_normalized_ = 0.0;
};
}
}
