#include "FlightControlActuationModel.h"

#include "Common/Clamp.h"

#include <cmath>
#include <stdexcept>

namespace
{
constexpr double kSaturationToleranceRad = 1.0e-3;

Core::FlightControlPositionLimit position_limit(
	double target_rad,
	double position_rad,
	double maximum_deflection_rad)
{
	if (target_rad > maximum_deflection_rad)
		return Core::FlightControlPositionLimit::Positive;
	if (target_rad < -maximum_deflection_rad)
		return Core::FlightControlPositionLimit::Negative;
	const bool positive_stop =
		position_rad >= maximum_deflection_rad - kSaturationToleranceRad &&
		target_rad >= position_rad;
	if (positive_stop) return Core::FlightControlPositionLimit::Positive;
	const bool negative_stop =
		position_rad <= -maximum_deflection_rad + kSaturationToleranceRad &&
		target_rad <= position_rad;
	return negative_stop ? Core::FlightControlPositionLimit::Negative
		: Core::FlightControlPositionLimit::None;
}

bool at_position_limit(
	Core::FlightControlPositionLimit limit,
	double position_rad,
	double maximum_deflection_rad)
{
	switch (limit)
	{
	case Core::FlightControlPositionLimit::None:
		return false;
	case Core::FlightControlPositionLimit::Negative:
		return position_rad <=
			-maximum_deflection_rad + kSaturationToleranceRad;
	case Core::FlightControlPositionLimit::Positive:
		return position_rad >=
			maximum_deflection_rad - kSaturationToleranceRad;
	}
	throw std::logic_error("Unknown flight-control position-limit state.");
}

struct SurfaceStateInput
{
	const Systems::FlightControlAxisModelState& state;
	const Core::Systems::FlightControlAxisConfig& config;
	const Systems::FlightControlAxisStepInput& step;
	double previous_position_rad = 0.0;
	bool rate_limited = false;
};

Core::FlightControlSurfaceState make_surface_state(
	const SurfaceStateInput& input)
{
	const Core::FlightControlPositionLimit limit = position_limit(
		input.step.target_position_rad, input.state.position_rad,
		input.config.maximum_deflection_rad);
	const bool stop_reached = at_position_limit(
		limit, input.state.position_rad,
		input.config.maximum_deflection_rad);
	return {
		input.state.position_rad,
		(input.state.position_rad - input.previous_position_rad) /
			input.step.dt_s,
		limit,
		stop_reached,
		input.rate_limited,
		input.rate_limited || limit != Core::FlightControlPositionLimit::None
	};
}
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
	const double target_rad = Common::limit(
		input.target_position_rad,
		-config.maximum_deflection_rad,
		config.maximum_deflection_rad);
	const double maximum_step = config.rate_limit_rad_s * input.dt_s;
	const bool rate_limited =
		std::fabs(target_rad - current.rate_limited_position_rad) >
			maximum_step + kSaturationToleranceRad;
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
		-config.maximum_deflection_rad,
		config.maximum_deflection_rad);
	return { next, make_surface_state(
		{ next, config, input, previous_position, rate_limited }) };
}
}
