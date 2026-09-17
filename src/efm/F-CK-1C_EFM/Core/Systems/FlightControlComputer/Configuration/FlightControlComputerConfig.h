#pragma once

#include "../CommandSystem/AutomaticFlightControlTypes.h"
#include "../ControlLaws/ControlLawConfig.h"

namespace Core
{
namespace Systems
{
struct FlightControlComputerConfig
{
	::Systems::InputSignalManagementConfig input_signal_management;
	::Systems::ModeAndGainSchedulingConfig mode_and_gain;
	::Systems::GuidanceCoordinationConfig guidance_coordination;
	::Systems::FlightControlLawsConfig flight_control_laws;
	::Systems::FlightControlOutputConfig flight_control_output;
	::Systems::FlightControlDiagnosticsConfig diagnostics;
	AutomaticFlightControlConfig automatic_flight_control;
	::Systems::FlightControlDevelopmentConfig development;
};

void validate_flight_control_computer_config(
	const FlightControlComputerConfig& config);
const FlightControlComputerConfig& fck1c_flight_control_computer_config();
}
}
