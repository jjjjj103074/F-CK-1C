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
	const auto elevator = ::Systems::update_flight_control_axis(
		elevator_, config_.elevator, { command.elevator_normalized, dt_s });
	const auto aileron = ::Systems::update_flight_control_axis(
		aileron_, config_.aileron, { command.aileron_normalized, dt_s });
	const auto rudder = ::Systems::update_flight_control_axis(
		rudder_, config_.rudder, { command.rudder_normalized, dt_s });
	elevator_ = elevator.model_state;
	aileron_ = aileron.model_state;
	rudder_ = rudder.model_state;
	state_.elevator = elevator.surface_state;
	state_.aileron = aileron.surface_state;
	state_.rudder = rudder.surface_state;
	state_.any_saturated = state_.elevator.saturated ||
		state_.aileron.saturated || state_.rudder.saturated;
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
