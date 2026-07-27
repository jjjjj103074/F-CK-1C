#pragma once

#include <functional>
#include <memory>

namespace Core
{
namespace Simulation
{
class AircraftSimulation;
struct FlightSetupContext;

using AircraftSimulationFactory = std::function<
	std::unique_ptr<AircraftSimulation>(const FlightSetupContext&)>;
}
}
