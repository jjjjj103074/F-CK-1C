#include "GuidanceCoordinationConfig.h"

#include "Common/ConfigValidation.h"

#include <stdexcept>

namespace Systems
{
void validate_guidance_coordination_config(
	const GuidanceCoordinationConfig& config)
{
	const bool valid = Common::all_finite({
		config.pitch_error_to_rate_gain_s_inv,
		config.vertical_speed_error_to_acceleration_gain_s_inv,
		config.bank_error_to_roll_rate_gain_s_inv,
		config.coordinated_turn_minimum_speed_mps }) &&
		config.pitch_error_to_rate_gain_s_inv > 0.0 &&
		config.vertical_speed_error_to_acceleration_gain_s_inv > 0.0 &&
		config.bank_error_to_roll_rate_gain_s_inv > 0.0 &&
		config.coordinated_turn_minimum_speed_mps > 0.0;
	if (!valid)
	{
		throw std::invalid_argument(
			"Invalid FLCC guidance coordination configuration.");
	}
}
}
