#include "FlightControlDiagnosticsConfig.h"

#include "Common/ConfigValidation.h"

#include <stdexcept>

namespace Systems
{
void validate_flight_control_diagnostics_config(
	const FlightControlDiagnosticsConfig& config)
{
	const bool valid = Common::all_finite({
		config.control_authority_persistence_s,
		config.minimum_alpha_recovery_rate_rad_s }) &&
		config.control_authority_persistence_s > 0.0 &&
		config.minimum_alpha_recovery_rate_rad_s >= 0.0;
	if (!valid)
	{
		throw std::invalid_argument(
			"Invalid FLCC diagnostics configuration.");
	}
}
}
