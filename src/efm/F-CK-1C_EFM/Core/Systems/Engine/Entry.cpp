#include "Engine.h"

#include <utility>

namespace Core
{
namespace Systems
{
SystemEntry make_engine_system_entry(const EngineConfig& config)
{
	validate_engine_config(config);
	return {
		"engine",
		SystemGroup::Equipment,
		[owned_config = config](const FlightSetupContext& setup)
		{
			return std::make_unique<Engine>(
				owned_config,
				setup.start_mode);
		}
	};
}

namespace Catalog
{
namespace Engine
{
SystemEntry create_entry()
{
	return make_engine_system_entry(fck1c_engine_config());
}
}
}
}
}
