#include "FlightControlComputer.h"

#include "../SystemPipeline.h"
#include "../SystemUpdateRates.h"
#include "ControlLaws/ConfigurationAndMode.h"
#include "ControlLaws/ControlLawMath.h"
#include "ControlLaws/PilotCommandLaw.h"
#include "Common/Table.h"

namespace
{
constexpr double kEnabledCommandThreshold = 0.5;
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
		fck1c_automatic_flight_control_config(),
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
	const AutomaticFlightGuidanceReference& automatic =
		automatic_flight_control_.step(
			make_automatic_observation(raw, input));
	const ::Systems::FlightControlLawStepInput control_law_input =
		make_control_law_input(input, automatic);
	const ::Systems::FlightControlLawResult output =
		::Systems::update_fbw_controller(
			fbw_, config_.control_laws, control_law_input);
	apply_experimental_auto_throttle(automatic);
	refresh_outputs(output, request.throttle_levers);
	refresh_diagnostics();
	return actuator_command_;
}

::Systems::FlightControlLawStepInput
FlightControlComputer::make_control_law_input(
	const ::Systems::ConditionedFlightControlInput& flight,
	const AutomaticFlightGuidanceReference& automatic)
{
	const ::Systems::FBWCatParams cat = ::Systems::fbw_blend_cat_params(
		config_.control_laws.cat1,
		config_.control_laws.cat3,
		flight.cat_mode_blend);
	const ::Systems::FBWGainScheduleValues gains =
		::Systems::fbw_eval_gain_schedule(
			config_.control_laws, flight.dynamic_pressure_pa);
	const ::Systems::ManeuverEnvelope envelope =
		::Systems::make_maneuver_envelope(
			config_.control_laws,
			{ cat, flight.alpha_limit_deg, fbw_.g_limiter_override });
	const CoordinatedManeuverReference manual =
		::Systems::make_pilot_maneuver_reference(
			{ flight, config_.control_laws, cat, gains, envelope });
	selected_reference_ = select_flight_reference(manual, automatic);
	coordinated_reference_ = coordinate_guidance(
		{ flight, selected_reference_, envelope, config_.control_laws });
	return { flight, coordinated_reference_.reference };
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
		observation.roll_rate_rad_s,
		observation.yaw_rate_rad_s,
		conditioned.pilot_pitch_normalized,
		raw.landing_gear.any_weight_on_wheels
	};
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
}
}
