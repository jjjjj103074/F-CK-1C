#include "FlightControlComputer.h"

namespace Core
{
namespace Systems
{
SystemEntry make_flight_control_computer_system_entry(
	const FlightControlComputerConfig& config)
{
	validate_flight_control_computer_config(config);
	return {
		"flight_control_computer",
		SystemGroup::Control,
		[owned_config = config](const FlightSetupContext& setup)
		{
			return std::make_unique<FlightControlComputer>(
				owned_config,
				setup.start_mode);
		}
	};
}

namespace Catalog
{
namespace FlightControlComputer
{
SystemEntry create_entry()
{
	return make_flight_control_computer_system_entry(
		fck1c_flight_control_computer_config());
}
}
}
}
}
