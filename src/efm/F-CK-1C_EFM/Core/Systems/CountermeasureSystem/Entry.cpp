#include "CountermeasureSystem.h"
namespace Core::Systems::Catalog::CountermeasureSystem
{
SystemEntry create_entry()
{
	return { "countermeasure_system",
		     [](const FlightSetupContext&)
		     {
		         return std::make_unique<::Core::Systems::CountermeasureSystem>();
		     } };
}
}
