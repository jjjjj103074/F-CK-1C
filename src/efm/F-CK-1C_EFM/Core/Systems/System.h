#pragma once

#include "../Contracts/Commands.h"
#include "../Contracts/Events.h"
#include "../Contracts/FrameContracts.h"

#include <functional>
#include <memory>
#include <string>

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

struct FlightSetupContext
{
	const StartMode start_mode = StartMode::ColdGround;
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
