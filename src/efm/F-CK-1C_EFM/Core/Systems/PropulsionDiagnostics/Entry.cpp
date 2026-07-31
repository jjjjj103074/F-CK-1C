#include "PropulsionDiagnostics.h"

#include <memory>

namespace Core
{
namespace Systems
{
namespace Catalog
{
namespace PropulsionDiagnostics
{
SystemEntry create_entry()
{
	return {
		"propulsion_diagnostics",
		SystemGroup::Equipment,
		[](const FlightSetupContext&)
		{
			return std::make_unique<
				Core::Systems::PropulsionDiagnostics>();
		}
	};
}
}
}
}
}
