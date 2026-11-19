#include "MissionComputer.h"

namespace Core::Systems::Catalog::MissionComputer
{
/// @brief 將 MC 登記至 System catalog。
SystemEntry create_entry()
{
	SystemEntry entry;
	entry.id = "mission_computer";
	entry.factory = [](const FlightSetupContext&)
	{
		return std::make_unique<Core::Systems::MissionComputer>();
	};
	return entry;
}
}
