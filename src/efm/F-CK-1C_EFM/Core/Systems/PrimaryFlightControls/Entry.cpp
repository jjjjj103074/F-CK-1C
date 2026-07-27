#include "PrimaryFlightControls.h"

namespace Core
{
namespace Systems
{
namespace Catalog
{
namespace PrimaryFlightControls
{
SystemEntry create_entry()
{
	return {
		"primary_flight_controls",
		SystemGroup::Equipment,
		[](const FlightSetupContext&)
		{
			return std::make_unique<Core::Systems::PrimaryFlightControls>();
		}
	};
}
}
}
}
}
