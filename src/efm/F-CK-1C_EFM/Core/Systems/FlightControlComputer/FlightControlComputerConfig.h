#pragma once

#include "Autopilot/AutomaticFlightControlTypes.h"
#include "ControlLaws/ControlLawConfig.h"

#include <vector>

namespace Core
{
namespace Systems
{
struct FlightControlComputerConfig
{
	::Systems::FBWControllerConfig control_laws;
	AutomaticFlightControlConfig automatic_flight_control;
	std::vector<double> mach_table;
	std::vector<double> alpha_limit_deg;
	bool developer_g_limiter_override_available = false;
};

void validate_flight_control_computer_config(
	const FlightControlComputerConfig& config);
const FlightControlComputerConfig& fck1c_flight_control_computer_config();
}
}
