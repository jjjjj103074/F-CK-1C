#include "FlightControlActuationSystem.h"

namespace Core
{
namespace Systems
{
namespace Catalog
{
namespace FlightControlActuationSystem
{
SystemEntry create_entry()
{
	return make_flight_control_actuation_system_entry(
		fck1c_flight_control_actuation_system_config());
}
}
}
}
}
