#include "LandingGear.h"

namespace Core
{
namespace Systems
{
SystemEntry make_landing_gear_system_entry(
	const LandingGearConfig& config)
{
	validate_landing_gear_config(config);
	return {
		"landing_gear",
		SystemGroup::Equipment,
		[owned_config = config](const FlightSetupContext& setup)
		{
			return std::make_unique<LandingGear>(
				setup.start_mode,
				owned_config);
		}
	};
}

namespace Catalog
{
namespace LandingGear
{
SystemEntry create_entry()
{
	return make_landing_gear_system_entry(
		fck1c_landing_gear_config());
}
}
}
}
}
