#include "FlightControlComputer.h"

#include "Data/AircraftConfig.h"

namespace Core
{
namespace Systems
{
namespace Catalog
{
namespace FlightControlComputer
{
SystemEntry create_entry()
{
	return {
		"flight_control_computer",
		SystemGroup::Control,
		[](const FlightSetupContext& setup)
		{
			const Data::AircraftConfig& config = setup.config;
			return std::make_unique<Core::Systems::FlightControlComputer>(
				config.fbw,
				FlightEnvelopeDefinition{
					config.aerodynamics.mach_table,
					config.aerodynamics.alpha_max_table
				},
				setup.start_mode);
		}
	};
}
}
}
}
}
