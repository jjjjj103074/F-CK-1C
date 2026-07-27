#include "AircraftSimulation.h"

namespace Core
{
namespace Simulation
{
void AircraftSimulation::handle_command(const Command& command)
{
	switch (command.group)
	{
	case CommandGroup::PitchRoll:
	case CommandGroup::Yaw:
	case CommandGroup::Fbw:
	case CommandGroup::Throttle:
		flight_control_computer_.handle_command(command);
		break;
	case CommandGroup::Engine:
		engine_.handle_command(command);
		break;
	case CommandGroup::Airframe:
		secondary_flight_controls_.handle_command(command);
		break;
	case CommandGroup::LandingGear:
		landing_gear_.handle_command(command);
		break;
	case CommandGroup::None:
		break;
	}
}
}
}
