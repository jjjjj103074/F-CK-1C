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

struct VerticalGuidanceReference
{
	VerticalGuidanceReferenceType type = VerticalGuidanceReferenceType::None;
	double pitch_attitude_rad = 0.0;
	double vertical_speed_mps = 0.0;
};

class VerticalGuidance
{
public:
	explicit VerticalGuidance(const AutomaticFlightControlConfig& config);
	VerticalGuidanceReference update(
		const AutomaticFlightControlObservation& observation,
		const VerticalGuidanceTarget& target,
		bool active);
	void reset();

private:
	VerticalGuidanceReference update_pitch_reference(
		const AutomaticFlightControlObservation& observation,
		double target_pitch_rad);
	VerticalGuidanceReference update_vertical_speed_reference(
		const AutomaticFlightControlObservation& observation,
		double target_vertical_speed_mps);
	VerticalGuidanceReference update_altitude_reference(
		const AutomaticFlightControlObservation& observation,
		double target_altitude_m);
	double rate_limit(
		double current,
		double target,
		double maximum_rate_per_s,
		double dt_s) const;
	double altitude_desired_vertical_speed(
		const AutomaticFlightControlObservation& observation,
		double altitude_error_m) const;
	double comfort_limited_vertical_speed(
		const AutomaticFlightControlObservation& observation,
		double altitude_error_m,
		double maximum_vertical_speed_mps,
		double remaining_distance_factor) const;

	const AutomaticFlightControlConfig config_;
	AutomaticFlightControlVerticalMode previous_mode_ =
		AutomaticFlightControlVerticalMode::Off;
	double pitch_reference_rad_ = 0.0;
	double vertical_speed_reference_mps_ = 0.0;
};
}
}
