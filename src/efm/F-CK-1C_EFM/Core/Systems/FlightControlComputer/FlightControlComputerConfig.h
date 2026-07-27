#pragma once

#include "ControlLawTypes.h"

#include <vector>

namespace Core
{
namespace Systems
{
struct FlightControlComputerConfig
{
	::Systems::FBWControllerConfig control_laws;
	std::vector<double> mach_table;
	std::vector<double> alpha_limit_deg;
};

void validate_flight_control_computer_config(
	const FlightControlComputerConfig& config);
const FlightControlComputerConfig& fck1c_flight_control_computer_config();
}
}
