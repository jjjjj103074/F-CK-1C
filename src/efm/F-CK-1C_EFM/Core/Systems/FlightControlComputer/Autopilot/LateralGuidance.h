#pragma once

#include "AutomaticFlightControlTypes.h"

namespace Core
{
namespace Systems
{
struct LateralGuidanceReference
{
	double bank_angle_rad = 0.0;
};

class LateralGuidance
{
public:
	explicit LateralGuidance(const AutomaticFlightControlConfig& config);
	LateralGuidanceReference update(
		const AutomaticFlightControlObservation& observation,
		double target_heading_rad,
		bool active);
	void reset();

private:
	const AutomaticFlightControlConfig config_;
	double heading_integral_ = 0.0;
	double bank_reference_rad_ = 0.0;
	bool active_last_tick_ = false;
};
}
}
