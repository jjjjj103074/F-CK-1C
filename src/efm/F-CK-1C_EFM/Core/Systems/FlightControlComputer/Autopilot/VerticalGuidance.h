#pragma once

#include "AutomaticFlightControlTypes.h"
#include "../../../Contracts/CockpitContracts.h"

namespace Core
{
namespace Systems
{
struct VerticalGuidanceTarget
{
	AutomaticFlightControlVerticalMode mode =
		AutomaticFlightControlVerticalMode::Off;
	double pitch_attitude_rad = 0.0;
	double vertical_speed_mps = 0.0;
	double altitude_m = 0.0;
};

class VerticalGuidance
{
public:
	explicit VerticalGuidance(const AutomaticFlightControlConfig& config);
	double update(
		const AutomaticFlightControlObservation& observation,
		const VerticalGuidanceTarget& target,
		bool active);
	void reset();

private:
	double update_pitch_hold(
		const AutomaticFlightControlObservation& observation,
		double target_pitch_rad) const;
	double update_vertical_speed_hold(
		const AutomaticFlightControlObservation& observation,
		double target_vertical_speed_mps);
	double update_altitude_hold(
		const AutomaticFlightControlObservation& observation,
		double target_altitude_m);
	double altitude_desired_vertical_speed(
		const AutomaticFlightControlObservation& observation,
		double altitude_error_m) const;
	double comfort_limited_vertical_speed(
		const AutomaticFlightControlObservation& observation,
		double altitude_error_m,
		double maximum_vertical_speed_mps,
		double remaining_distance_factor) const;

	const AutomaticFlightControlConfig config_;
	double vertical_speed_integral_ = 0.0;
};
}
}
