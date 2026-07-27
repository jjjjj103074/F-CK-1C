#include "Engine.h"

#include "Data/AircraftConfig.h"

namespace Core
{
namespace Systems
{
namespace Catalog
{
namespace Engine
{
SystemEntry create_entry()
{
	return {
		"engine",
		SystemGroup::Equipment,
		[](const FlightSetupContext& setup)
		{
			const Data::AircraftConfig& config =
				Data::fck1c_aircraft_config();
			return std::make_unique<Core::Systems::Engine>(
				config.engine,
				config.fuel.consumption_rate,
				setup.start_mode);
		}
	};
}
}
}
}
}
