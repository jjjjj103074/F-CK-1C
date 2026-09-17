#include "FlightControlCommandSystem.h"

#include "Internal/Autopilot/AutomaticFlightControl.h"
#include "Internal/ControlReferenceSelection.h"
#include "Internal/GuidanceCoordination.h"
#include "Internal/PilotCommandLaw.h"
#include "../Configuration/FlightControlComputerConfig.h"

namespace Core
{
namespace Systems
{
class FlightControlCommandSystem::Implementation final
{
public:
	Implementation(
		const FlightControlComputerConfig& config,
		bool initial_weight_on_wheels)
		: longitudinal_config_(config.flight_control_laws.longitudinal),
		  coordination_config_(config.guidance_coordination),
		  automatic_(config.automatic_flight_control,
			  initial_weight_on_wheels,
			  config.development.experimental_auto_throttle_available)
	{
	}

	void register_commands(SystemSetup& setup)
	{
		automatic_.register_commands(setup);
	}

	bool handles(CommandId id) const
	{
		return AutomaticFlightControl::handles(id);
	}

	void handle_command(const Command& command)
	{
		automatic_.handle_command(command);
	}

	const FlightControlCommandSystemResult& update(
		const FlightControlCommandSystemInput& input)
	{
		result_.automatic = automatic_.step(
			make_automatic_observation(input), input.configuration.envelope);
		const auto manual = ::Systems::make_pilot_maneuver_reference({
			input.signals, longitudinal_config_, input.configuration.stores,
			input.configuration.gains, input.configuration.envelope });
		result_.selected = select_flight_reference(manual, result_.automatic);
		result_.coordinated = coordinate_guidance({
			input.flight, result_.selected, input.configuration.envelope,
			coordination_config_, input.signals.landing_gear_handle_down });
		return result_;
	}

	void observe_control_result(
		const FlightControlCommandMonitorInput& input)
	{
		automatic_.observe_control_result({
			input.dt_s, input.vertical_active, input.lateral_active,
			input.vertical_type, input.pitch_tracking_error_rad,
			input.vertical_speed_tracking_error_ft_s,
			input.lateral_tracking_error_rad, input.constraint,
			input.control_path_saturated, input.hard_protection_reason });
	}

	const AutomaticFlightControlSnapshot& automatic_snapshot() const
	{
		return automatic_.snapshot();
	}

private:
	AutomaticFlightControlObservation make_automatic_observation(
		const FlightControlCommandSystemInput& input) const
	{
		return {
			input.flight.dt_s, input.flight.indicated_airspeed_mps,
			input.flight.pressure_altitude_ft,
			input.flight.vertical_speed_ft_s, input.flight.mach,
			input.flight.magnetic_heading_available,
			input.flight.magnetic_heading_deg,
			input.flight.pitch_attitude_rad, input.flight.roll_attitude_rad,
			input.signals.pilot_pitch_normalized,
			input.signals.pilot_roll_normalized,
			input.signals.weight_on_wheels,
			input.flight.pressure_altitude_available
		};
	}

	const ::Systems::LongitudinalControlConfig longitudinal_config_;
	const ::Systems::GuidanceCoordinationConfig coordination_config_;
	AutomaticFlightControl automatic_;
	FlightControlCommandSystemResult result_;
};

FlightControlCommandSystem::FlightControlCommandSystem(
	const FlightControlComputerConfig& config,
	bool initial_weight_on_wheels)
	: implementation_(
		new Implementation(config, initial_weight_on_wheels))
{
}

FlightControlCommandSystem::~FlightControlCommandSystem() = default;

void FlightControlCommandSystem::register_commands(SystemSetup& setup)
{
	implementation_->register_commands(setup);
}

bool FlightControlCommandSystem::handles(CommandId id) const
{
	return implementation_->handles(id);
}

void FlightControlCommandSystem::handle_command(const Command& command)
{
	implementation_->handle_command(command);
}

const FlightControlCommandSystemResult& FlightControlCommandSystem::update(
	const FlightControlCommandSystemInput& input)
{
	return implementation_->update(input);
}

void FlightControlCommandSystem::observe_control_result(
	const FlightControlCommandMonitorInput& observation)
{
	implementation_->observe_control_result(observation);
}

const AutomaticFlightControlSnapshot&
FlightControlCommandSystem::automatic_snapshot() const
{
	return implementation_->automatic_snapshot();
}
}
}
