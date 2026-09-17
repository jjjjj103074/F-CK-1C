#include "FlightControlActuationSystemConfig.h"

#include "Common/Units.h"

#include <cmath>
#include <stdexcept>

namespace
{
bool valid_axis(const Core::Systems::FlightControlAxisConfig& axis)
{
	return std::isfinite(axis.maximum_deflection_rad) &&
		std::isfinite(axis.rate_limit_rad_s) &&
		std::isfinite(axis.lag_time_constant_s) &&
		axis.maximum_deflection_rad > 0.0 &&
		axis.rate_limit_rad_s > 0.0 &&
		axis.lag_time_constant_s > 0.0;
}

Core::Systems::FlightControlActuationSystemConfig make_fck1c_config()
{
	// Project-defined values migrated from the legacy FCC actuator model.
	// Public F-CK-1C actuator limits and dynamics have not been confirmed.
	return {
		{ Common::rad(25.0), Common::rad(120.0), 0.04 },
		{ Common::rad(22.0), Common::rad(110.0), 0.05 },
		{ Common::rad(30.0), Common::rad(80.0), 0.07 }
	};
}
}

namespace Core
{
namespace Systems
{
void validate_flight_control_actuation_system_config(
	const FlightControlActuationSystemConfig& config)
{
	if (!valid_axis(config.symmetric_stabilator) ||
		!valid_axis(config.differential_flaperon) ||
		!valid_axis(config.rudder))
	{
		throw std::invalid_argument(
			"FlightControlActuationSystemConfig requires positive, "
			"finite axis limits and dynamics.");
	}
}

const FlightControlActuationSystemConfig&
	fck1c_flight_control_actuation_system_config()
{
	static const FlightControlActuationSystemConfig config =
		make_fck1c_config();
	return config;
}
}
}
