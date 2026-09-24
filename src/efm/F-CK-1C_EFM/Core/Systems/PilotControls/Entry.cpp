#include "PilotControls.h"

namespace Core
{
namespace Systems
{
namespace Catalog
{
namespace PilotControls
{
SystemEntry create_entry()
{
	return {
		"pilot_controls",
		[](const FlightSetupContext& setup)
		{
			return std::make_unique<Core::Systems::PilotControls>(
				setup.initial_throttle_levers);
		}
	};
}
}
}
}
}
