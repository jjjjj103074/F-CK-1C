#include "FlightControlActuationModel.h"

#include "Common/Clamp.h"

#include <cmath>
#include <stdexcept>

namespace
{
constexpr double kSaturationToleranceRad = 1.0e-3;
}

namespace Systems
{
FlightControlAxisStepResult update_flight_control_axis(
	const FlightControlAxisModelState& current,
	const Core::Systems::FlightControlAxisConfig& config,
	const FlightControlAxisStepInput& input)
{
	if (!std::isfinite(input.dt_s) || input.dt_s <= 0.0)
	{
		throw std::invalid_argument(
			"Flight-control actuator dt must be positive and finite.");
	}
	FlightControlAxisModelState next = current;
	const double limited_command = Common::limit(
		input.normalized_command, -1.0, 1.0);
	const double target_rad = limited_command * config.travel_limit_rad;
	const double maximum_step = config.rate_limit_rad_s * input.dt_s;
	next.rate_limited_position_rad += Common::limit(
		target_rad - next.rate_limited_position_rad,
		-maximum_step,
		maximum_step);
	const double previous_position = next.position_rad;
	const double lag_gain = Common::limit(
		input.dt_s / (config.lag_time_constant_s + input.dt_s), 0.0, 1.0);
	next.position_rad +=
		(next.rate_limited_position_rad - next.position_rad) * lag_gain;
	next.position_rad = Common::limit(
		next.position_rad,
		-config.travel_limit_rad,
		config.travel_limit_rad);
	const bool saturated =
		std::fabs(input.normalized_command - limited_command) > 0.0 ||
		std::fabs(target_rad - next.rate_limited_position_rad) >
			kSaturationToleranceRad ||
		std::fabs(next.position_rad) >
			config.travel_limit_rad - kSaturationToleranceRad;
	return {
		next,
		{
			next.position_rad,
			(next.position_rad - previous_position) / input.dt_s,
			Common::limit(
				next.position_rad / config.travel_limit_rad, -1.0, 1.0),
			saturated
		}
	};
}
}
