#pragma once

#include "AutomaticFlightControlTypes.h"

namespace Core
{
namespace Systems
{
class LateralGuidance
{
public:
	explicit LateralGuidance(const AutomaticFlightControlConfig& config);
	double update(
		const AutomaticFlightControlObservation& observation,
		double target_heading_rad,
		bool active);
	void reset();

private:
	const AutomaticFlightControlConfig config_;
	double heading_integral_ = 0.0;
};
}
}
