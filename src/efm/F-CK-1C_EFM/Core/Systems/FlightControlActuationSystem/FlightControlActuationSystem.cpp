#include "FlightControlActuationSystem.h"

#include "../SystemPipeline.h"
#include "../SystemUpdateRates.h"

namespace Core
{
namespace Systems
{
FlightControlActuationSystem::FlightControlActuationSystem(
	const FlightControlActuationSystemConfig& config)
	: config_(config)
{
	validate_flight_control_actuation_system_config(config_);
}

void FlightControlActuationSystem::setup(SystemSetup& setup)
{
	setup.update_rate_hz(kFlightControlActuationProjectUpdateRateHz);
	setup.read(AircraftDataKeys::kFlightControlActuatorCommand);
	setup.publish(AircraftDataKeys::kFlightControlActuatorState, state_);
}

void FlightControlActuationSystem::step(
	const SystemStepContext& context,
	const AircraftDataView& aircraft,
	SystemResult& result)
{
	result.publish(
		AircraftDataKeys::kFlightControlActuatorState,
		update(
			aircraft.read(AircraftDataKeys::kFlightControlActuatorCommand),
			context.dt_s));
}

const FlightControlActuatorState& FlightControlActuationSystem::update(
	const FlightControlActuatorCommand& command,
	double dt_s)
{
	const auto stabilator = ::Systems::update_flight_control_axis(
		symmetric_stabilator_, config_.symmetric_stabilator,
		{ command.symmetric_stabilator_demand_rad, dt_s });
	const auto flaperon = ::Systems::update_flight_control_axis(
		differential_flaperon_, config_.differential_flaperon,
		{ command.differential_flaperon_demand_rad, dt_s });
	const auto rudder = ::Systems::update_flight_control_axis(
		rudder_, config_.rudder, { command.rudder_demand_rad, dt_s });
	symmetric_stabilator_ = stabilator.model_state;
	differential_flaperon_ = flaperon.model_state;
	rudder_ = rudder.model_state;
	state_.symmetric_stabilator = stabilator.surface_state;
	state_.differential_flaperon = flaperon.surface_state;
	state_.rudder = rudder.surface_state;
	state_.any_saturated = state_.symmetric_stabilator.saturated ||
		state_.differential_flaperon.saturated || state_.rudder.saturated;
	return state_;
}

const FlightControlActuatorState&
	FlightControlActuationSystem::state() const
{
	return state_;
}

SystemEntry make_flight_control_actuation_system_entry(
	const FlightControlActuationSystemConfig& config)
{
	validate_flight_control_actuation_system_config(config);
	return {
		"flight_control_actuation_system",
		SystemGroup::Equipment,
		[owned_config = config](const FlightSetupContext&)
		{
			return std::make_unique<FlightControlActuationSystem>(
				owned_config);
		}
	};
}
}
}
