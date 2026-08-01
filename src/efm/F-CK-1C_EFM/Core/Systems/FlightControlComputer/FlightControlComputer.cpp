#include "FlightControlComputer.h"

#include "../SystemPipeline.h"
#include "../SystemUpdateRates.h"
#include "ControlLaws/ConfigurationAndMode.h"
#include "ControlLaws/ControlLawMath.h"
#include "ControlLaws/PilotCommandLaw.h"
#include "Common/Table.h"

#include <stdexcept>

namespace
{
constexpr double kEnabledCommandThreshold = 0.5;

Core::FlightControlVerticalReferenceType to_snapshot_reference_type(
	Core::Systems::VerticalGuidanceReferenceType type)
{
	switch (type)
	{
	case Core::Systems::VerticalGuidanceReferenceType::None:
		return Core::FlightControlVerticalReferenceType::None;
	case Core::Systems::VerticalGuidanceReferenceType::PitchAttitude:
		return Core::FlightControlVerticalReferenceType::PitchAttitude;
	case Core::Systems::VerticalGuidanceReferenceType::VerticalSpeed:
		return Core::FlightControlVerticalReferenceType::VerticalSpeed;
	}
	throw std::logic_error("Unknown vertical guidance reference type.");
}
}

namespace Core
{
namespace Systems
{
FlightControlComputer::FlightControlComputer(
	const FlightControlComputerConfig& config,
	StartMode start_mode,
	const ThrottleLeverSignal& initial_throttle_levers)
	: config_(config),
	input_signals_(config.control_laws),
	automatic_flight_control_(
		config.automatic_flight_control,
		start_mode != StartMode::HotAir)
{
	validate_flight_control_computer_config(config_);
	::Systems::reset_fbw_state(fbw_, {});
	engine_throttle_command_ = {
		initial_throttle_levers.left_normalized,
		initial_throttle_levers.right_normalized
	};
	diagnostics_.status.available = true;
	diagnostics_.developer_g_limiter_override_available =
		config_.developer_g_limiter_override_available;
}

void FlightControlComputer::setup(SystemSetup& setup)
{
	setup.update_rate_hz(kF16XlDflcsReferenceUpdateRateHz);
	setup.read(AircraftDataKeys::kFlightControlObservation);
	setup.read(AircraftDataKeys::kPilotControlSignal);
	setup.read(AircraftDataKeys::kThrottleLeverSignal);
	setup.read(AircraftDataKeys::kLandingGearData);
	setup.read(AircraftDataKeys::kFlightControlActuatorState);
	setup.publish(
		AircraftDataKeys::kFlightControlActuatorCommand,
		actuator_command_);
	setup.publish(
		AircraftDataKeys::kEngineThrottleCommand,
		engine_throttle_command_);
	setup.publish(
		AircraftDataKeys::kAutomaticFlightControlSnapshot,
		automatic_flight_control_.snapshot());
	setup.publish(
		AircraftDataKeys::kFlightControlComputerSnapshot,
		diagnostics_);
	register_commands(setup);
}

void FlightControlComputer::register_commands(SystemSetup& setup)
{
	const CommandId commands[] = {
		CommandId::ToggleFbwCat,
		CommandId::SetFbwCat1,
		CommandId::SetFbwCat3,
		CommandId::SetGLimiterOverride,
		CommandId::ToggleGLimiterOverride
	};
	for (CommandId id : commands)
	{
		setup.register_command_handler(
			id,
			[this](const Command& command) { handle_command(command); });
	}
	automatic_flight_control_.register_commands(setup);
}

void FlightControlComputer::step(
	const SystemStepContext& context,
	const AircraftDataView& aircraft,
	SystemResult& result)
{
	step({
		make_pipeline_input(context, aircraft),
		aircraft.read(AircraftDataKeys::kThrottleLeverSignal)
	});
	result.publish(
		AircraftDataKeys::kFlightControlActuatorCommand,
		actuator_command_);
	result.publish(
		AircraftDataKeys::kEngineThrottleCommand,
		engine_throttle_command_);
	result.publish(
		AircraftDataKeys::kAutomaticFlightControlSnapshot,
		automatic_flight_control_.snapshot());
	result.publish(
		AircraftDataKeys::kFlightControlComputerSnapshot,
		diagnostics_);
}

const FlightControlActuatorCommand& FlightControlComputer::step(
	const FlightControlComputerStepInput& request)
{
	RawFlightControlInput raw = request.flight_control;
	raw.alpha_limit_deg = alpha_limit(raw.observation.mach);
	::Systems::ConditionedFlightControlInput input =
		input_signals_.condition(raw, fbw_.mode_target);
	const ::Systems::FlightControlConfiguration configuration =
		make_configuration(input);
	const AutomaticFlightGuidanceReference& automatic =
		automatic_flight_control_.step(
			make_automatic_observation(raw, input), configuration.envelope);
	const ::Systems::FlightControlLawStepInput control_law_input =
		make_control_law_input(input, automatic, configuration);
	const ::Systems::FlightControlLawResult output =
		::Systems::update_fbw_controller(
			fbw_, config_.control_laws, control_law_input);
	automatic_flight_control_.observe_control_result(
		make_mode_monitor_observation(input, automatic));
	apply_experimental_auto_throttle(automatic);
	refresh_outputs(output, request.throttle_levers);
	refresh_diagnostics();
	return actuator_command_;
}

::Systems::FlightControlConfiguration
FlightControlComputer::make_configuration(
	const ::Systems::ConditionedFlightControlInput& flight) const
{
	return ::Systems::make_flight_control_configuration(
		config_.control_laws,
		{ flight.cat_mode_blend,
			flight.dynamic_pressure_pa,
			flight.alpha_limit_deg,
			fbw_.g_limiter_override });
}

::Systems::FlightControlLawStepInput
FlightControlComputer::make_control_law_input(
	const ::Systems::ConditionedFlightControlInput& flight,
	const AutomaticFlightGuidanceReference& automatic,
	const ::Systems::FlightControlConfiguration& configuration)
{
	const CoordinatedManeuverReference manual =
		::Systems::make_pilot_maneuver_reference(
			{ flight, config_.control_laws, configuration.cat,
				configuration.gains, configuration.envelope });
	selected_reference_ = select_flight_reference(manual, automatic);
	coordinated_reference_ = coordinate_guidance(
		{ flight, selected_reference_, configuration.envelope,
			config_.control_laws });
	return { flight, coordinated_reference_.reference, configuration };
}

void FlightControlComputer::apply_experimental_auto_throttle(
	const AutomaticFlightGuidanceReference& automatic)
{
	if (!automatic.experimental_auto_throttle_engaged)
	{
		fbw_.throttle_blend = 0.0;
		return;
	}
	fbw_.throttle_cmd_left = automatic.experimental_throttle_normalized;
	fbw_.throttle_cmd_right = automatic.experimental_throttle_normalized;
	fbw_.throttle_blend = 1.0;
	fbw_.throttle_override = false;
}

void FlightControlComputer::refresh_outputs(
	const ::Systems::FlightControlLawResult& output,
	const ThrottleLeverSignal& throttle_levers)
{
	actuator_command_ = {
		output.surface_demand.elevator_command_normalized,
		output.surface_demand.aileron_command_normalized,
		output.surface_demand.rudder_command_normalized
	};
	engine_throttle_command_ = {
		::Systems::compose_engine_throttle_command({
			throttle_levers.left_normalized,
			fbw_.throttle_cmd_left,
			fbw_.throttle_blend,
			fbw_.throttle_override
		}),
		::Systems::compose_engine_throttle_command({
			throttle_levers.right_normalized,
			fbw_.throttle_cmd_right,
			fbw_.throttle_blend,
			fbw_.throttle_override
		})
	};
}

void FlightControlComputer::refresh_diagnostics()
{
	++diagnostics_.status.revision;
	diagnostics_.developer_g_limiter_override_active =
		fbw_.g_limiter_override;
	refresh_selected_reference_diagnostics();
	const CoordinatedManeuverReference& reference =
		coordinated_reference_.reference;
	diagnostics_.normal_acceleration_reference_g =
		reference.longitudinal.normal_acceleration_reference_g;
	diagnostics_.pitch_rate_feedforward_rad_s =
		reference.longitudinal.pitch_rate_feedforward_rad_s;
	diagnostics_.roll_rate_reference_rad_s =
		reference.lateral_directional.roll_rate_reference_rad_s;
	diagnostics_.sideslip_reference_rad =
		reference.lateral_directional.sideslip_reference_rad;
	diagnostics_.yaw_rate_feedforward_rad_s =
		reference.lateral_directional.yaw_rate_feedforward_rad_s;
	diagnostics_.elevator_command_normalized =
		actuator_command_.elevator_normalized;
	diagnostics_.aileron_command_normalized =
		actuator_command_.aileron_normalized;
	diagnostics_.rudder_command_normalized =
		actuator_command_.rudder_normalized;
	diagnostics_.constraint_reason = coordinated_reference_.constraint.reason;
	diagnostics_.vertical_constrained =
		coordinated_reference_.constraint.vertical_constrained;
	diagnostics_.lateral_constrained =
		coordinated_reference_.constraint.lateral_constrained;
}

void FlightControlComputer::refresh_selected_reference_diagnostics()
{
	diagnostics_.selected_normal_acceleration_reference_g = 0.0;
	diagnostics_.selected_pitch_rate_feedforward_rad_s = 0.0;
	diagnostics_.selected_pitch_attitude_reference_rad = 0.0;
	diagnostics_.selected_vertical_speed_reference_mps = 0.0;
	diagnostics_.selected_roll_rate_reference_rad_s = 0.0;
	diagnostics_.selected_bank_angle_reference_rad = 0.0;
	refresh_selected_longitudinal_diagnostics();
	refresh_selected_lateral_diagnostics();
	diagnostics_.selected_directional_source =
		FlightControlReferenceSource::Manual;
	diagnostics_.selected_sideslip_reference_rad =
		selected_reference_.directional.sideslip_reference_rad;
	diagnostics_.selected_yaw_rate_feedforward_rad_s =
		selected_reference_.directional.yaw_rate_feedforward_rad_s;
}

void FlightControlComputer::refresh_selected_longitudinal_diagnostics()
{
	if (const auto* manual = std::get_if<LongitudinalManeuverReference>(
		&selected_reference_.longitudinal))
	{
		diagnostics_.selected_longitudinal_source =
			FlightControlReferenceSource::Manual;
		diagnostics_.selected_vertical_reference_type =
			FlightControlVerticalReferenceType::None;
		diagnostics_.selected_normal_acceleration_reference_g =
			manual->normal_acceleration_reference_g;
		diagnostics_.selected_pitch_rate_feedforward_rad_s =
			manual->pitch_rate_feedforward_rad_s;
	}
	else
	{
		const auto& automatic =
			std::get<AutomaticLongitudinalFlightReference>(
				selected_reference_.longitudinal);
		diagnostics_.selected_longitudinal_source =
			FlightControlReferenceSource::Automatic;
		diagnostics_.selected_vertical_reference_type =
			to_snapshot_reference_type(automatic.type);
		diagnostics_.selected_pitch_attitude_reference_rad =
			automatic.pitch_attitude_reference_rad;
		diagnostics_.selected_vertical_speed_reference_mps =
			automatic.vertical_speed_reference_mps;
	}
}

void FlightControlComputer::refresh_selected_lateral_diagnostics()
{
	if (const auto* manual = std::get_if<ManualLateralFlightReference>(
		&selected_reference_.lateral))
	{
		diagnostics_.selected_lateral_source =
			FlightControlReferenceSource::Manual;
		diagnostics_.selected_roll_rate_reference_rad_s =
			manual->roll_rate_reference_rad_s;
	}
	else
	{
		diagnostics_.selected_lateral_source =
			FlightControlReferenceSource::Automatic;
		diagnostics_.selected_bank_angle_reference_rad =
			std::get<AutomaticLateralFlightReference>(
				selected_reference_.lateral).bank_angle_reference_rad;
	}
}

RawFlightControlInput FlightControlComputer::make_pipeline_input(
	const SystemStepContext& context,
	const AircraftDataView& aircraft) const
{
	const FlightControlObservation& observation =
		aircraft.read(AircraftDataKeys::kFlightControlObservation);
	const LandingGearData& gear =
		aircraft.read(AircraftDataKeys::kLandingGearData);
	const FlightControlActuatorState& actuator =
		aircraft.read(AircraftDataKeys::kFlightControlActuatorState);
	return {
		context.dt_s,
		alpha_limit(observation.mach),
		observation,
		aircraft.read(AircraftDataKeys::kPilotControlSignal),
		gear,
		actuator
	};
}

AutomaticFlightControlObservation
FlightControlComputer::make_automatic_observation(
	const RawFlightControlInput& raw,
	const ::Systems::ConditionedFlightControlInput& conditioned) const
{
	const FlightControlObservation& observation = raw.observation;
	return {
		raw.dt_s,
		observation.indicated_airspeed_mps,
		observation.altitude_asl_m,
		observation.vertical_speed_mps,
		observation.mach,
		observation.heading_rad,
		observation.pitch_rad,
		observation.roll_rad,
		conditioned.pilot_pitch_normalized,
		conditioned.pilot_roll_normalized,
		raw.landing_gear.any_weight_on_wheels
	};
}

AutopilotModeMonitorObservation
FlightControlComputer::make_mode_monitor_observation(
	const ::Systems::ConditionedFlightControlInput& flight,
	const AutomaticFlightGuidanceReference& automatic) const
{
	double vertical_error = 0.0;
	if (automatic.vertical_type ==
		VerticalGuidanceReferenceType::PitchAttitude)
	{
		vertical_error = automatic.pitch_attitude_reference_rad -
			flight.pitch_attitude_rad;
	}
	else if (automatic.vertical_type ==
		VerticalGuidanceReferenceType::VerticalSpeed)
	{
		vertical_error = automatic.vertical_speed_reference_mps -
			flight.vertical_speed_mps;
	}
	return {
		flight.dt_s,
		automatic.longitudinal_authority == AuthorityState::Automatic,
		automatic.lateral_authority == AuthorityState::Automatic,
		automatic.vertical_type,
		vertical_error,
		automatic.bank_angle_reference_rad - flight.roll_attitude_rad,
		coordinated_reference_.constraint,
		flight.actuator_saturated,
		hard_protection_reason()
	};
}

ConstraintReason FlightControlComputer::hard_protection_reason() const
{
	if (fbw_.aoa_limit_active) return ConstraintReason::HardAngleOfAttackLimit;
	if (fbw_.g_limit_active) return ConstraintReason::HardLoadFactorLimit;
	if (fbw_.rate_limit_active) return ConstraintReason::HardRateLimit;
	return ConstraintReason::None;
}

double FlightControlComputer::alpha_limit(double mach) const
{
	return Common::lerp(
		{
			config_.mach_table.data(),
			config_.alpha_limit_deg.data(),
			static_cast<unsigned>(config_.mach_table.size())
		},
		mach);
}

void FlightControlComputer::handle_command(const Command& command)
{
	const bool enabled = command.value > kEnabledCommandThreshold;
	switch (command.id)
	{
	case CommandId::ToggleFbwCat:
		::Systems::toggle_fbw_cat_mode(fbw_, enabled); break;
	case CommandId::SetFbwCat1:
		if (enabled) ::Systems::set_fbw_cat_mode(fbw_, ::Systems::FBW_CAT1);
		break;
	case CommandId::SetFbwCat3:
		if (enabled) ::Systems::set_fbw_cat_mode(fbw_, ::Systems::FBW_CAT3);
		break;
	case CommandId::SetGLimiterOverride:
		::Systems::set_fbw_g_limiter_override(
			fbw_, config_.developer_g_limiter_override_available && enabled);
		break;
	case CommandId::ToggleGLimiterOverride:
		if (config_.developer_g_limiter_override_available)
		{
			::Systems::toggle_fbw_g_limiter_override(fbw_, enabled);
		}
		else
		{
			::Systems::set_fbw_g_limiter_override(fbw_, false);
		}
		break;
	default:
		if (AutomaticFlightControl::handles(command.id))
			automatic_flight_control_.handle_command(command);
		break;
	}
}

const FlightControlActuatorCommand&
	FlightControlComputer::actuator_command() const
{
	return actuator_command_;
}

const EngineThrottleCommand&
FlightControlComputer::engine_throttle_command() const
{
	return engine_throttle_command_;
}

const FlightControlComputerSnapshot&
FlightControlComputer::diagnostics() const
{
	return diagnostics_;
}

const AutomaticFlightControlSnapshot&
FlightControlComputer::automatic_flight_control_snapshot() const
{
	return automatic_flight_control_.snapshot();
}
}
}
