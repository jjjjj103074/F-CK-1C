#pragma once

#include "../Contracts/Commands.h"
#include "../Contracts/Events.h"
#include "../Contracts/FrameContracts.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Data
{
struct AircraftConfig;
}

namespace Core
{
namespace Systems
{
class AircraftDataSnapshot;
class SystemResult;
class SystemSetup;

enum class SystemGroup
{
	Control,
	Equipment
};

struct FlightFuelState
{
	double internal_fuel = 0.0;
	std::vector<ExternalFuelInput> external_fuel;
};

struct FlightSetupContext
{
	const Data::AircraftConfig& config;
	const StartMode start_mode;
	const FlightFuelState fuel;
};

class System
{
public:
	virtual ~System() = default;

	virtual void setup(SystemSetup& setup) = 0;
	virtual void step(
		const AircraftDataSnapshot& snapshot,
		SystemResult& result) = 0;
};

using SystemFactory =
	std::function<std::unique_ptr<System>(const FlightSetupContext&)>;

struct SystemEntry
{
	std::string id;
	SystemGroup group = SystemGroup::Equipment;
	SystemFactory factory;
};
}
}
