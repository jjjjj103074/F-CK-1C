#include "Fuel.h"

namespace Core
{
namespace Systems
{
namespace Catalog
{
namespace Fuel
{
SystemEntry create_entry()
{
	return {
		"fuel",
		SystemGroup::Equipment,
		[](const FlightSetupContext& setup)
		{
			return std::make_unique<Core::Systems::Fuel>(setup.fuel);
		}
	};
}
}
}
}
}
