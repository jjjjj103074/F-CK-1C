#include "AirframeStructure.h"

namespace Core
{
namespace Systems
{
namespace Catalog
{
namespace AirframeStructure
{
SystemEntry create_entry()
{
	return {
		"airframe_structure",
		SystemGroup::Equipment,
		[](const FlightSetupContext&)
		{
			return std::make_unique<Core::Systems::AirframeStructure>();
		}
	};
}
}
}
}
}
