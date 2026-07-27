#include "AircraftSimulation.h"

namespace Core
{
namespace Simulation
{
void AircraftSimulation::handle_command(const Command& command)
{
	(void)system_pipeline_.send(command);
}
}
}
