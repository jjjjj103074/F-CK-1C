#include "InputSignalManagementConfig.h"

#include "Common/ConfigValidation.h"

#include <stdexcept>

namespace Systems
{
void validate_input_signal_management_config(
	const InputSignalManagementConfig& config)
{
	const bool valid = Common::all_finite({
		config.signal_filter_time_constant_s,
		config.dynamic_pressure_filter_time_constant_s,
		config.normal_acceleration_filter_time_constant_s }) &&
		config.signal_filter_time_constant_s > 0.0 &&
		config.dynamic_pressure_filter_time_constant_s > 0.0 &&
		config.normal_acceleration_filter_time_constant_s > 0.0;
	if (!valid)
	{
		throw std::invalid_argument(
			"Invalid FLCC input-signal configuration.");
	}
}
}
