#include "RadarSystem.h"
namespace Core::Systems::Catalog::RadarSystem
{
SystemEntry create_entry()
{
	SystemEntry entry;
	entry.id = "radar_system";
	entry.factory = [](const FlightSetupContext&)
	{
		return std::make_unique<::Core::Systems::RadarSystem>();
	};
	return entry;
}
}
