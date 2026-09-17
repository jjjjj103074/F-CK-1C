#include "InnerLoopControl.h"

// Control-law implementation detail; use FlightControlLaws.

#include "Common/Clamp.h"

#include <cmath>

namespace
{
constexpr double kAntiWindupTolerance = 1.0e-4;
}

namespace Systems
{
AxisRateLoopResult update_axis_rate_loop(
	double current_integral,
	const AxisRateLoopStepInput& input)
{
	const double error_rad_s =
		input.reference_rad_s - input.measured_rad_s;
	const double unsaturated =
		input.gains.proportional * error_rad_s +
		input.gains.integral * current_integral;
	const double effort = Common::limit(unsaturated, -1.0, 1.0);
	const double next_integral = current_integral +
		(error_rad_s + input.gains.anti_windup *
			(effort - unsaturated)) * input.dt_s;
	return {
		Common::limit(next_integral,
			-input.gains.integral_limit,
			input.gains.integral_limit),
		effort,
		error_rad_s,
		std::fabs(unsaturated - effort) > kAntiWindupTolerance
	};
}
}
