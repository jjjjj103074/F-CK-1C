#include "SecondaryFlightControls.h"

namespace Core
{
namespace Systems
{
namespace Catalog
{
namespace SecondaryFlightControls
{
SystemEntry create_entry()
{
	return {
		"secondary_flight_controls",
		SystemGroup::Equipment,
		[](const FlightSetupContext& setup)
		{
			return std::make_unique<Core::Systems::SecondaryFlightControls>(
				setup.start_mode);
		}
	};
}
}
}
}
}
