#pragma once

#include "FlightControlActuationSystemConfig.h"
#include "../../Contracts/AircraftData.h"

namespace Systems
{
struct FlightControlAxisModelState
{
	double rate_limited_position_rad = 0.0;
	double position_rad = 0.0;
};

struct FlightControlAxisStepInput
{
	double target_position_rad = 0.0;
	double dt_s = 0.0;
};

struct FlightControlAxisStepResult
{
	FlightControlAxisModelState model_state;
	Core::FlightControlSurfaceState surface_state;
};

FlightControlAxisStepResult update_flight_control_axis(
	const FlightControlAxisModelState& current,
	const Core::Systems::FlightControlAxisConfig& config,
	const FlightControlAxisStepInput& input);
}
