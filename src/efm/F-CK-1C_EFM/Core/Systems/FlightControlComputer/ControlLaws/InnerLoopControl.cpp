#include "InnerLoopControl.h"

#include "Common/Clamp.h"

#include <cmath>

namespace
{
constexpr double kConstraintTolerance = 1e-5;
constexpr double kAntiWindupTolerance = 1e-4;

struct IntegralStep
{
	double current;
	double error;
	double unsaturated_command;
	double saturated_command;
	double dt_s;
	double anti_windup;
	double limit;
};

struct InnerLoopComputation
{
	Systems::BodyRateReference error;
	Systems::NormalizedControlSurfaceDemand demand;
	double roll_unsaturated = 0.0;
	double pitch_unsaturated = 0.0;
	double yaw_unsaturated = 0.0;
};

double integrate(const IntegralStep& input)
{
	const double next = input.current +
		(input.error + input.anti_windup *
			(input.saturated_command - input.unsaturated_command)) * input.dt_s;
	return Common::limit(next, -input.limit, input.limit);
}

InnerLoopComputation compute_commands(
	const Systems::InnerRateLoopState& current,
	const Systems::InnerRateLoopStepInput& input)
{
	InnerLoopComputation result;
	result.error = {
		input.reference.roll_rate_rad_s - input.measured.roll_rate_rad_s,
		input.reference.pitch_rate_rad_s - input.measured.pitch_rate_rad_s,
		input.reference.yaw_rate_rad_s - input.measured.yaw_rate_rad_s
	};
	result.roll_unsaturated =
		input.gains.roll_proportional * result.error.roll_rate_rad_s +
		input.gains.roll_integral * current.roll_integral;
	result.pitch_unsaturated =
		input.gains.pitch_proportional * result.error.pitch_rate_rad_s +
		input.gains.pitch_integral * current.pitch_integral;
	result.yaw_unsaturated =
		input.gains.yaw_proportional * result.error.yaw_rate_rad_s +
		input.gains.yaw_integral * current.yaw_integral;
	result.demand = {
		Common::limit(result.pitch_unsaturated, -1.0, 1.0),
		Common::limit(result.roll_unsaturated, -1.0, 1.0),
		Common::limit(result.yaw_unsaturated, -1.0, 1.0)
	};
	return result;
}

Systems::InnerRateLoopState integrate_state(
	const Systems::InnerRateLoopState& current,
	const Systems::InnerRateLoopStepInput& input,
	const InnerLoopComputation& computation)
{
	return {
		integrate({ current.roll_integral,
			computation.error.roll_rate_rad_s,
			computation.roll_unsaturated,
			computation.demand.aileron_command_normalized,
			input.dt_s, input.gains.anti_windup, input.gains.integral_limit }),
		integrate({ current.pitch_integral,
			computation.error.pitch_rate_rad_s,
			computation.pitch_unsaturated,
			computation.demand.elevator_command_normalized,
			input.dt_s, input.gains.anti_windup, input.gains.integral_limit }),
		integrate({ current.yaw_integral,
			computation.error.yaw_rate_rad_s,
			computation.yaw_unsaturated,
			computation.demand.rudder_command_normalized,
			input.dt_s, input.gains.anti_windup, input.gains.integral_limit })
	};
}

bool anti_windup_active(const InnerLoopComputation& computation)
{
	return std::fabs(computation.roll_unsaturated -
		computation.demand.aileron_command_normalized) > kAntiWindupTolerance ||
		std::fabs(computation.pitch_unsaturated -
			computation.demand.elevator_command_normalized) >
			kAntiWindupTolerance ||
		std::fabs(computation.yaw_unsaturated -
			computation.demand.rudder_command_normalized) > kAntiWindupTolerance;
}
}

namespace Systems
{
LimitedBodyRateReference limit_body_rate_reference(
	const BodyRateReference& reference,
	const BodyRateLimit& limit)
{
	LimitedBodyRateReference result;
	result.value = {
		Common::limit(
			reference.roll_rate_rad_s,
			-limit.roll_rate_rad_s,
			limit.roll_rate_rad_s),
		Common::limit(
			reference.pitch_rate_rad_s,
			-limit.pitch_rate_rad_s,
			limit.pitch_rate_rad_s),
		Common::limit(
			reference.yaw_rate_rad_s,
			-limit.yaw_rate_rad_s,
			limit.yaw_rate_rad_s)
	};
	result.constrained =
		std::fabs(result.value.roll_rate_rad_s - reference.roll_rate_rad_s) >
			kConstraintTolerance ||
		std::fabs(result.value.pitch_rate_rad_s - reference.pitch_rate_rad_s) >
			kConstraintTolerance ||
		std::fabs(result.value.yaw_rate_rad_s - reference.yaw_rate_rad_s) >
			kConstraintTolerance;
	return result;
}

InnerRateLoopResult update_inner_rate_loop(
	const InnerRateLoopState& current,
	const InnerRateLoopStepInput& input)
{
	const InnerLoopComputation computation = compute_commands(current, input);
	return {
		integrate_state(current, input, computation),
		computation.demand,
		computation.error,
		anti_windup_active(computation)
	};
}
}
