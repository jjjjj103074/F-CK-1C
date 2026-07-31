#pragma once

namespace Core
{
namespace Systems
{
struct FlightControlAxisConfig
{
	double travel_limit_rad = 0.0;
	double rate_limit_rad_s = 0.0;
	double lag_time_constant_s = 0.0;
};

struct FlightControlActuationSystemConfig
{
	FlightControlAxisConfig elevator;
	FlightControlAxisConfig aileron;
	FlightControlAxisConfig rudder;
};

void validate_flight_control_actuation_system_config(
	const FlightControlActuationSystemConfig& config);
const FlightControlActuationSystemConfig&
	fck1c_flight_control_actuation_system_config();
}
}
