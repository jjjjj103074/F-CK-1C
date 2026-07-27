#include "LandingGear.h"

#include "Data/AircraftConfig.h"

namespace Core
{
namespace Systems
{
namespace Catalog
{
namespace LandingGear
{
SystemEntry create_entry()
{
	return {
		"landing_gear",
		SystemGroup::Equipment,
		[](const FlightSetupContext& setup)
		{
			return std::make_unique<Core::Systems::LandingGear>(
				setup.start_mode,
				Data::fck1c_aircraft_config().suspension);
		}
	};
}
}
}
}
}
