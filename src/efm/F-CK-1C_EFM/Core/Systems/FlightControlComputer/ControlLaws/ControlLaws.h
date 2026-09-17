#pragma once

#include "ControlLawConfig.h"
#include "ControlLawSignals.h"
#include "ControlLawState.h"

namespace Systems
{
class FlightControlLaws
{
public:
	explicit FlightControlLaws(const FlightControlLawsConfig& config);
	FlightControlLawsResult update(const FlightControlLawsInput& input);
	void reset();

private:
	const FlightControlLawsConfig config_;
	LongitudinalControlLawState longitudinal_;
	LateralControlLawState lateral_;
	DirectionalControlLawState directional_;
};
}
