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
			return std::make_unique<Core::Systems::FlightControlComputer>(
				Data::fck1c_aircraft_config().fbw,
				FlightEnvelopeDefinition{
					Data::fck1c_aircraft_config().aerodynamics.mach_table,
					Data::fck1c_aircraft_config().aerodynamics.alpha_max_table
				},
				setup.start_mode);
		}
	};
}
}
}
}
}
