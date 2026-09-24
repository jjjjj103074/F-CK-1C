#include "FlightControlOutputConfig.h"

#include "Common/ConfigValidation.h"

#include <stdexcept>

namespace Systems
{
void validate_flight_control_output_config(
	const FlightControlOutputConfig& config)
{
	const bool valid = Common::all_finite({
		config.selection_transition_time_s, config.tracking_tolerance_rad }) &&
		config.selection_transition_time_s > 0.0 &&
		config.tracking_tolerance_rad > 0.0;
	if (!valid)
	{
		throw std::invalid_argument("Invalid FLCC output configuration.");
	}
}
}
