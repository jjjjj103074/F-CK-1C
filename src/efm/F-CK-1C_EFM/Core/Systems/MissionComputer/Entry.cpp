#include "MissionComputer.h"

namespace Core::Systems::Catalog::MissionComputer
{
/// @brief 登記單一 MC；內部元件不加入 System catalog。
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
