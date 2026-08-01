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
	struct RateLimitInput
	{
		double current = 0.0;
		double target = 0.0;
		double maximum_rate_per_s = 0.0;
		double dt_s = 0.0;
	};
	struct ComfortLimitInput
	{
		double altitude_error_m = 0.0;
		double maximum_vertical_speed_mps = 0.0;
		double remaining_distance_factor = 0.0;
	};
	VerticalGuidanceReference update_pitch_reference(
		const AutomaticFlightControlObservation& observation,
		double target_pitch_rad);
	VerticalGuidanceReference update_vertical_speed_reference(
		const AutomaticFlightControlObservation& observation,
		double target_vertical_speed_mps);
	VerticalGuidanceReference update_altitude_reference(
		const AutomaticFlightControlObservation& observation,
		double target_altitude_m);
	double rate_limit(const RateLimitInput& input) const;
	double altitude_desired_vertical_speed(
		const AutomaticFlightControlObservation& observation,
		double altitude_error_m) const;
	double comfort_limited_vertical_speed(
		const AutomaticFlightControlObservation& observation,
		const ComfortLimitInput& input) const;

	const AutomaticFlightControlConfig config_;
	AutomaticFlightControlVerticalMode previous_mode_ =
		AutomaticFlightControlVerticalMode::Off;
	double pitch_reference_rad_ = 0.0;
	double vertical_speed_reference_mps_ = 0.0;
};
}
}
