#include "FlightControlOutputSystem.h"

#include "Common/Clamp.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace
{
constexpr double kSaturationToleranceRad = 1.0e-12;

struct TrackingErrors
{
	double stabilator_rad = 0.0;
	double maximum_rad = 0.0;
};

struct ElectronicLimitStatus
{
	bool stabilator = false;
	bool any = false;
};

void require_finite(double value, const char* field)
{
	if (!std::isfinite(value)) throw std::domain_error(field);
}

void validate_demand(const Systems::ControlSurfaceDemandSet& demand)
{
	require_finite(demand.symmetric_stabilator_demand_rad,
		"Non-finite FLCC stabilator demand.");
	require_finite(demand.differential_flaperon_demand_rad,
		"Non-finite FLCC flaperon demand.");
	require_finite(demand.rudder_demand_rad,
		"Non-finite FLCC rudder demand.");
}

void validate_feedback(const Core::FlightControlActuatorState& actuator)
{
	const Core::FlightControlSurfaceState surfaces[] = {
		actuator.symmetric_stabilator,
		actuator.differential_flaperon,
		actuator.rudder
	};
	for (const auto& surface : surfaces)
	{
		require_finite(surface.position_rad,
			"Non-finite flight-control actuator position.");
		require_finite(surface.rate_rad_s,
			"Non-finite flight-control actuator rate.");
	}
}

double blend(double source, double target, double amount)
{
	return source + (target - source) * amount;
}

TrackingErrors tracking_errors(
	const Core::FlightControlActuatorCommand& command,
	const Core::FlightControlActuatorState& actuator)
{
	const double stabilator = std::fabs(
		command.symmetric_stabilator_demand_rad -
		actuator.symmetric_stabilator.position_rad);
	const double flaperon = std::fabs(
		command.differential_flaperon_demand_rad -
		actuator.differential_flaperon.position_rad);
	const double rudder = std::fabs(
		command.rudder_demand_rad - actuator.rudder.position_rad);
	return { stabilator, (std::max)({ stabilator, flaperon, rudder }) };
}

bool demand_was_limited(double command_rad, double demand_rad)
{
	return std::fabs(command_rad - demand_rad) > kSaturationToleranceRad;
}

ElectronicLimitStatus electronic_limit_status(
	const Core::FlightControlActuatorCommand& command,
	const Systems::ControlSurfaceDemandSet& demand)
{
	const bool stabilator = demand_was_limited(
		command.symmetric_stabilator_demand_rad,
		demand.symmetric_stabilator_demand_rad);
	const bool flaperon = demand_was_limited(
		command.differential_flaperon_demand_rad,
		demand.differential_flaperon_demand_rad);
	const bool rudder = demand_was_limited(
		command.rudder_demand_rad, demand.rudder_demand_rad);
	return { stabilator, stabilator || flaperon || rudder };
}

bool longitudinal_protection_active(
	const Systems::FlightControlLawsStatus& status)
{
	return status.angle_of_attack_limit_active ||
		status.load_factor_limit_active || status.body_rate_limit_active;
}

bool stabilator_at_commanded_position_limit(
	const Core::Systems::FlightControlOutputInput& input,
	const Core::FlightControlActuatorCommand& command)
{
	const auto& actuator = input.actuator.symmetric_stabilator;
	if (!actuator.at_position_limit) return false;
	const double demand = command.symmetric_stabilator_demand_rad;
	switch (actuator.position_limit)
	{
	case Core::FlightControlPositionLimit::None:
		return false;
	case Core::FlightControlPositionLimit::Negative:
		return demand <= actuator.position_rad;
	case Core::FlightControlPositionLimit::Positive:
		return demand >= actuator.position_rad;
	}
	throw std::logic_error("Unknown stabilator position-limit state.");
}
}

namespace Core
{
namespace Systems
{
FlightControlOutputSystem::FlightControlOutputSystem(
	const ::Systems::SurfaceCommandMixerConfig& limits,
	const ::Systems::FlightControlOutputConfig& config)
	: limits_(limits), config_(config)
{
}

::Systems::ControlSurfaceDemandSet
FlightControlOutputSystem::selected_demand(
	const FlightControlOutputInput& input) const
{
	return input.selection == ElectronicControlLawSelection::DeveloperDirect
		? input.laws.developer_direct_surface_demand
		: input.laws.normal_surface_demand;
}

::Systems::ControlSurfaceDemandSet
FlightControlOutputSystem::transition_demand(
	const ::Systems::ControlSurfaceDemandSet& target,
	double dt_s)
{
	if (transition_0_1_ >= 1.0) return target;
	transition_0_1_ = Common::limit(transition_0_1_ +
		dt_s / config_.selection_transition_time_s, 0.0, 1.0);
	return {
		blend(transition_source_.symmetric_stabilator_demand_rad,
			target.symmetric_stabilator_demand_rad, transition_0_1_),
		blend(transition_source_.differential_flaperon_demand_rad,
			target.differential_flaperon_demand_rad, transition_0_1_),
		blend(transition_source_.rudder_demand_rad,
			target.rudder_demand_rad, transition_0_1_)
	};
}

FlightControlActuatorCommand FlightControlOutputSystem::bounded_command(
	const ::Systems::ControlSurfaceDemandSet& demand) const
{
	return {
		Common::limit(demand.symmetric_stabilator_demand_rad,
			-limits_.symmetric_stabilator_limit_rad,
			limits_.symmetric_stabilator_limit_rad),
		Common::limit(demand.differential_flaperon_demand_rad,
			-limits_.differential_flaperon_limit_rad,
			limits_.differential_flaperon_limit_rad),
		Common::limit(demand.rudder_demand_rad,
			-limits_.rudder_limit_rad, limits_.rudder_limit_rad)
	};
}

FlightControlOutputStatus FlightControlOutputSystem::make_status(
	const FlightControlOutputInput& input,
	const FlightControlActuatorCommand& command) const
{
	const TrackingErrors errors = tracking_errors(command, input.actuator);
	const ElectronicLimitStatus output_limits =
		electronic_limit_status(command, current_demand_);
	const bool law_saturated =
		selection_ == ElectronicControlLawSelection::Normal &&
		input.laws.status.electronic_command_saturated;
	const bool electronic_saturated = output_limits.any || law_saturated;
	const bool tracking_consistent =
		errors.maximum_rad <= config_.tracking_tolerance_rad;
	const bool stabilator_tracking_consistent =
		errors.stabilator_rad <= config_.tracking_tolerance_rad;
	const bool protection_authority_exhausted =
		longitudinal_protection_active(input.laws.status) &&
		(output_limits.stabilator || law_saturated ||
			(stabilator_at_commanded_position_limit(input, command) &&
				!stabilator_tracking_consistent));
	return { selection_, transition_0_1_ < 1.0,
		true, true, electronic_saturated, input.actuator.any_saturated,
		protection_authority_exhausted, tracking_consistent,
		errors.maximum_rad };
}

FlightControlOutputResult FlightControlOutputSystem::update(
	const FlightControlOutputInput& input)
{
	validate_demand(input.laws.normal_surface_demand);
	validate_demand(input.laws.developer_direct_surface_demand);
	validate_feedback(input.actuator);
	require_finite(input.dt_s, "Non-finite FLCC output dt.");
	if (input.dt_s <= 0.0) throw std::out_of_range("Invalid FLCC output dt.");
	const ::Systems::ControlSurfaceDemandSet target = selected_demand(input);
	if (!initialized_)
	{
		selection_ = input.selection;
		current_demand_ = target;
		initialized_ = true;
	}
	else if (selection_ != input.selection)
	{
		selection_ = input.selection;
		transition_source_ = current_demand_;
		transition_0_1_ = 0.0;
	}
	current_demand_ = transition_demand(target, input.dt_s);
	const FlightControlActuatorCommand command =
		bounded_command(current_demand_);
	return { command, make_status(input, command) };
}
}
}
