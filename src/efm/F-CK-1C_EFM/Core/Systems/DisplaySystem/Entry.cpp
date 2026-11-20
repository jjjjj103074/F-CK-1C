#include "DisplaySystem.h"

namespace Core::Systems::Catalog::DisplaySystem
{
SystemEntry create_entry()
{
	return { "display_system",
		     [](const FlightSetupContext&)
		     {
		         return std::make_unique<::Core::Systems::DisplaySystem>();
		     } };
}
}
