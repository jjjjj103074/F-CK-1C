#include "FlightControlExecutive.h"

#include "../SystemPipeline.h"

namespace
{
constexpr double kCommandEnabledThreshold = 0.5;
constexpr std::uint64_t kPilotShapingDivisor = 2;
constexpr std::uint64_t kGainScheduleDivisor = 16;
constexpr double kPilotShapingDtS = 1.0 / 32.0;

}

namespace Core
{
namespace Systems
{
FlightControlExecutive::FlightControlExecutive(
	const FlightControlComputerConfig& config,
	StartMode start_mode,
	const ThrottleLeverSignal& initial_throttle_levers)
	: config_(config),
	  input_signals_(config.input_signal_management),
	  mode_and_gain_(config.mode_and_gain),
	  command_system_(config, start_mode != StartMode::HotAir),
	  flight_control_laws_(config.flight_control_laws),
	  output_system_(config.flight_control_laws.surface_mixer,
		config.flight_control_output),
	  diagnostics_(config.development, config.diagnostics),
	  held_pilot_shaping_(config.mode_and_gain.cat1.pilot_input)
{
	validate_flight_control_computer_config(config_);
	result_.engine_throttle_command = {
		initial_throttle_levers.left_normalized,
		initial_throttle_levers.right_normalized
	};
	result_.diagnostics = diagnostics_.snapshot();
	result_.automatic_flight_control = command_system_.automatic_snapshot();
}

void FlightControlExecutive::register_commands(SystemSetup& setup)
{
	// CAT 與開發用限制解除由此層分派；自動飛行指令交給指令模組。
	const CommandId commands[] = {
		CommandId::ToggleFbwCat, CommandId::SetFbwCat1,
		CommandId::SetFbwCat3, CommandId::SetGLimiterOverride,
		CommandId::ToggleGLimiterOverride
	};
	for (CommandId id : commands)
	{
		setup.register_command_handler(
			id,
			[this](const SystemActionContext&, const Command& command)
			{ handle_command(command); });
	}
	command_system_.register_commands(setup);
}

void FlightControlExecutive::handle_command(const Command& command)
{
	const bool enabled = command.value_normalized > kCommandEnabledThreshold;
	switch (command.id)
	{
	case CommandId::ToggleFbwCat:
		mode_and_gain_.toggle_stores_configuration(enabled); return;
	case CommandId::SetFbwCat1:
		if (enabled) mode_and_gain_.set_stores_configuration(
			::Systems::StoresConfiguration::Cat1);
		return;
	case CommandId::SetFbwCat3:
		if (enabled) mode_and_gain_.set_stores_configuration(
			::Systems::StoresConfiguration::Cat3);
		return;
	case CommandId::SetGLimiterOverride:
		developer_g_limiter_override_active_ =
			config_.development.g_limiter_override_available && enabled;
		return;
	case CommandId::ToggleGLimiterOverride:
		if (enabled && config_.development.g_limiter_override_available)
			developer_g_limiter_override_active_ =
				!developer_g_limiter_override_active_;
		return;
	default:
		if (command_system_.handles(command.id))
			command_system_.handle_command(command);
	}
}

const FlightControlComputerResult& FlightControlExecutive::update(
	const FlightControlComputerStepInput& input)
{
	// 以 64 Hz 主週期決定較慢模組是否更新；其餘週期沿用保留值。
	const bool shaping_tick = tick_ % kPilotShapingDivisor == 0;
	const bool gain_tick = tick_ % kGainScheduleDivisor == 0;
	const auto& signals = input_signals_.update({
		input.flight_control, held_pilot_shaping_,
		shaping_tick, kPilotShapingDtS });
	const auto flight = compute_flight_state(signals);
	const auto& configuration = mode_and_gain_.update({
		flight.dt_s, flight.dynamic_pressure_pa,
		flight.mach, gain_tick,
		developer_g_limiter_override_active_,
		config_.development.g_limiter_override_margin_g,
		signals.landing_gear_handle_down });
	held_pilot_shaping_ = configuration.stores.pilot_input;
	const auto& command = command_system_.update(
		{ flight, signals, configuration });
	// 指令先形成控制需求，再由控制律與電子輸出層生成致動器命令。
	const auto laws = flight_control_laws_.update({
		flight, signals, command.coordinated.reference, configuration });
	const auto selection = config_.development.direct_control_law
		? ElectronicControlLawSelection::DeveloperDirect
		: ElectronicControlLawSelection::Normal;
	const auto output = output_system_.update({ laws, selection,
		input.flight_control.actuator, flight.dt_s });
	result_.actuator_command = output.actuator_command;
	const bool output_saturated =
		output.status.electronic_command_saturated ||
		output.status.actuator_saturated;
	// AP 監測本次已完成的控制結果；不回頭重算同一週期的控制律。
	command_system_.observe_control_result(make_monitor_observation(
		{ flight, command, laws, output_saturated }));
	update_engine_throttle(input.throttle_levers, command.automatic);
	update_subrate_counters(shaping_tick, gain_tick);
	update_diagnostics({ command, laws, configuration, signals, output.status });
	result_.automatic_flight_control = command_system_.automatic_snapshot();
	++tick_;
	return result_;
}

void FlightControlExecutive::update_subrate_counters(
	bool shaping_tick,
	bool gain_tick)
{
	if (shaping_tick)
	{
		++pilot_shaping_update_count_;
		pilot_shaping_last_update_tick_ = tick_;
	}
	if (gain_tick)
	{
		++gain_schedule_update_count_;
		gain_schedule_last_update_tick_ = tick_;
	}
}

void FlightControlExecutive::update_engine_throttle(
	const ThrottleLeverSignal& levers,
	const AutomaticFlightGuidanceReference& automatic)
{
	const double blend = automatic.experimental_auto_throttle_engaged
		? 1.0 : 0.0;
	result_.engine_throttle_command = {
		::Systems::compose_engine_throttle_command({ levers.left_normalized,
			automatic.experimental_throttle_normalized, blend, false }),
		::Systems::compose_engine_throttle_command({ levers.right_normalized,
			automatic.experimental_throttle_normalized, blend, false })
	};
}

void FlightControlExecutive::update_diagnostics(
	const FlightControlExecutiveDiagnosticsInput& input)
{
	result_.diagnostics = diagnostics_.update({
		tick_,
		pilot_shaping_update_count_, pilot_shaping_last_update_tick_,
		gain_schedule_update_count_, gain_schedule_last_update_tick_,
		mode_and_gain_.target_stores_configuration() ==
			::Systems::StoresConfiguration::Cat3,
		developer_g_limiter_override_active_, input.command, input.laws,
		input.configuration, input.signals, input.output_status,
		result_.actuator_command
	});
}

FlightControlCommandMonitorInput
FlightControlExecutive::make_monitor_observation(
	const AutopilotMonitorInput& input) const
{
	const double pitch_error_rad =
		input.command.automatic.pitch_attitude_reference_rad -
		input.flight.pitch_attitude_rad;
	const double vertical_speed_error_ft_s =
		input.command.automatic.vertical_speed_reference_ft_s -
		input.flight.vertical_speed_ft_s;
	return {
		input.flight.dt_s,
		input.command.automatic.longitudinal_authority == AuthorityState::Automatic,
		input.command.automatic.lateral_authority == AuthorityState::Automatic,
		input.command.automatic.vertical_type,
		pitch_error_rad,
		vertical_speed_error_ft_s,
		input.command.automatic.bank_angle_reference_rad -
			input.flight.roll_attitude_rad,
		input.command.coordinated.constraint,
		input.control_path_saturated,
		hard_protection_reason(input.laws.status)
	};
}

ConstraintReason FlightControlExecutive::hard_protection_reason(
	const ::Systems::FlightControlLawsStatus& status) const
{
	if (status.angle_of_attack_limit_active)
		return ConstraintReason::HardAngleOfAttackLimit;
	if (status.load_factor_limit_active)
		return ConstraintReason::HardLoadFactorLimit;
	if (status.body_rate_limit_active)
		return ConstraintReason::HardRateLimit;
	return ConstraintReason::None;
}

const FlightControlComputerResult& FlightControlExecutive::result() const
{
	return result_;
}
}
}
