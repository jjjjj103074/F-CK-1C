#include "FlightControlComputerConfig.h"

#include "../CommandSystem/AutomaticFlightControlTypes.h"
#include "../ModeAndGainScheduling/ModeAndGainScheduling.h"
#include "Common/ConfigValidation.h"
#include "Common/Units.h"

#include <algorithm>
#include <stdexcept>

namespace
{
Core::Systems::FlightControlComputerConfig make_fck1c_config()
{
	Core::Systems::FlightControlComputerConfig config;
	config.automatic_flight_control =
		Core::Systems::fck1c_automatic_flight_control_config(
			config.mode_and_gain);
	return config;
}

bool valid_input_config(const Systems::InputSignalManagementConfig& config)
{
	return Common::all_finite({ config.signal_filter_time_constant_s,
		config.dynamic_pressure_filter_time_constant_s,
		config.normal_acceleration_filter_time_constant_s }) &&
		config.signal_filter_time_constant_s > 0.0 &&
		config.dynamic_pressure_filter_time_constant_s > 0.0 &&
		config.normal_acceleration_filter_time_constant_s > 0.0;
}

bool valid_pilot_input(const Systems::PilotInputShapingConfig& config)
{
	return Common::all_finite({ config.deadband_normalized,
		config.command_time_constant_s, config.command_rate_normalized_s,
		config.cubic_weight }) && config.deadband_normalized >= 0.0 &&
		config.deadband_normalized < 1.0 &&
		config.command_time_constant_s > 0.0 &&
		config.command_rate_normalized_s > 0.0 &&
		config.cubic_weight >= 0.0 && config.cubic_weight <= 1.0;
}

bool valid_stores_envelope(
	const Systems::ManeuverEnvelopeSchedule& envelope)
{
	return Common::all_finite({
		envelope.maximum_roll_command_rad_s,
		envelope.maximum_yaw_command_rad_s,
		envelope.soft_positive_load_factor_g,
		envelope.hard_positive_load_factor_g,
		envelope.roll_rate_limit_rad_s,
		envelope.pitch_rate_limit_rad_s,
		envelope.yaw_rate_limit_rad_s }) &&
		envelope.maximum_roll_command_rad_s > 0.0 &&
		envelope.maximum_yaw_command_rad_s > 0.0 &&
		envelope.soft_positive_load_factor_g > 0.0 &&
		envelope.hard_positive_load_factor_g >
			envelope.soft_positive_load_factor_g &&
		envelope.roll_rate_limit_rad_s > 0.0 &&
		envelope.pitch_rate_limit_rad_s > 0.0 &&
		envelope.yaw_rate_limit_rad_s > 0.0;
}

bool valid_directional_schedule(
	const Systems::DirectionalControlSchedule& directional)
{
	return Common::all_finite({ directional.sideslip_damping_s_inv,
		directional.yaw_rate_damping }) &&
		directional.sideslip_damping_s_inv >= 0.0 &&
		directional.yaw_rate_damping >= 0.0;
}

bool valid_stores_schedule(const Systems::StoresControlLawSchedule& schedule)
{
	return valid_pilot_input(schedule.pilot_input) &&
		valid_stores_envelope(schedule.envelope) &&
		valid_directional_schedule(schedule.directional);
}

bool valid_gain_schedule(const Systems::ModeAndGainSchedulingConfig& config)
{
	double previous_pressure_pa = -1.0;
	for (const Systems::GainSchedulePoint& point : config.gain_schedule)
	{
		const bool valid = Common::all_finite({ point.dynamic_pressure_pa,
			point.command_gain, point.damping_gain, point.limiter_gain }) &&
			point.dynamic_pressure_pa > previous_pressure_pa &&
			point.command_gain > 0.0 && point.damping_gain > 0.0 &&
			point.limiter_gain > 0.0;
		if (!valid) return false;
		previous_pressure_pa = point.dynamic_pressure_pa;
	}
	return true;
}

bool valid_angle_of_attack_schedule(
	const Systems::ModeAndGainSchedulingConfig& config)
{
	return Common::finite_strictly_increasing(config.angle_of_attack_mach) &&
		config.angle_of_attack_limit_rad.size() ==
			config.angle_of_attack_mach.size() &&
		Common::all_finite(config.angle_of_attack_limit_rad) &&
		std::all_of(config.angle_of_attack_limit_rad.begin(),
			config.angle_of_attack_limit_rad.end(),
			[](double value) { return value > 0.0; });
}

bool valid_mode_and_gain_scalars(
	const Systems::ModeAndGainSchedulingConfig& config)
{
	return Common::all_finite({ config.stores_transition_time_constant_s,
			config.guidance_bank_limit_rad,
			config.guidance_roll_rate_limit_rad_s,
			config.guidance_minimum_load_factor_g,
			config.guidance_maximum_load_factor_g,
			config.hard_bank_limit_rad,
			config.hard_minimum_load_factor_g,
			config.cruise_angle_of_attack_blend_start_rad,
			config.landing_angle_of_attack_blend_start_rad,
			config.landing_angle_of_attack_limit_rad }) &&
		config.stores_transition_time_constant_s > 0.0 &&
		config.cruise_angle_of_attack_blend_start_rad > 0.0 &&
		config.landing_angle_of_attack_blend_start_rad > 0.0 &&
		config.landing_angle_of_attack_limit_rad >
			config.landing_angle_of_attack_blend_start_rad;
}

bool cruise_limits_exceed_blend_start(
	const Systems::ModeAndGainSchedulingConfig& config)
{
	return std::all_of(config.angle_of_attack_limit_rad.begin(),
			config.angle_of_attack_limit_rad.end(),
			[&config](double limit_rad)
			{
				return limit_rad >
					config.cruise_angle_of_attack_blend_start_rad;
			});
}

bool valid_mode_and_gain(const Systems::ModeAndGainSchedulingConfig& config)
{
	return valid_stores_schedule(config.cat1) &&
		valid_stores_schedule(config.cat3) &&
		valid_gain_schedule(config) &&
		valid_angle_of_attack_schedule(config) &&
		valid_mode_and_gain_scalars(config) &&
		cruise_limits_exceed_blend_start(config);
}

bool coherent_guidance_envelope(
	const Core::Systems::FlightControlComputerConfig& config)
{
	return config.automatic_flight_control.bank_limit_rad ==
			config.mode_and_gain.guidance_bank_limit_rad &&
		config.automatic_flight_control.roll_reference_rate_rad_s ==
			config.mode_and_gain.guidance_roll_rate_limit_rad_s;
}

bool coherent_alpha_control(
	const Core::Systems::FlightControlComputerConfig& config)
{
	const double terminal_g = config.flight_control_laws.longitudinal.
		angle_of_attack_limited_normal_acceleration_g;
	return terminal_g <
		config.mode_and_gain.cat1.envelope.hard_positive_load_factor_g &&
		terminal_g <
		config.mode_and_gain.cat3.envelope.hard_positive_load_factor_g;
}

bool finite_longitudinal_law(
	const Systems::LongitudinalControlConfig& config)
{
	return Common::all_finite({ config.positive_buffer_minimum_g,
		config.negative_soft_minimum_g, config.negative_soft_ratio,
		config.normal_acceleration_proportional_cat1,
		config.normal_acceleration_proportional_cat3,
		config.normal_acceleration_integral_cat1_s_inv,
		config.normal_acceleration_integral_cat3_s_inv,
		config.normal_acceleration_anti_windup_s_inv,
		config.normal_acceleration_integral_limit_effort,
		config.pitch_rate_washout_time_constant_s,
		config.pitch_rate_feedback_gain_s,
		config.angle_of_attack_stability_gain_rad_inv,
		config.pitch_rate_command_proportional_s,
		config.pitch_rate_command_integral_gain_rad_inv,
		config.pitch_rate_command_anti_windup_s_inv,
		config.pitch_rate_command_integral_limit_effort,
		config.angle_of_attack_limited_normal_acceleration_g,
		config.limit_buffer_bias_g,
		config.landing_pitch_rate_limit_rad_s });
}

bool valid_normal_acceleration_law(
	const Systems::LongitudinalControlConfig& config)
{
	return config.positive_buffer_minimum_g > 0.0 &&
		config.negative_soft_minimum_g > 0.0 &&
		config.negative_soft_ratio > 0.0 &&
		config.negative_soft_ratio <= 1.0 &&
		config.normal_acceleration_proportional_cat1 > 0.0 &&
		config.normal_acceleration_proportional_cat3 > 0.0;
}

bool valid_normal_acceleration_integrator(
	const Systems::LongitudinalControlConfig& config)
{
	return config.normal_acceleration_integral_cat1_s_inv >= 0.0 &&
		config.normal_acceleration_integral_cat3_s_inv >= 0.0 &&
		config.normal_acceleration_anti_windup_s_inv >= 0.0 &&
		config.normal_acceleration_integral_limit_effort > 0.0;
}

bool valid_pitch_rate_law(
	const Systems::LongitudinalControlConfig& config)
{
	return config.pitch_rate_washout_time_constant_s > 0.0 &&
		config.pitch_rate_feedback_gain_s > 0.0 &&
		config.angle_of_attack_stability_gain_rad_inv > 0.0 &&
		config.pitch_rate_command_proportional_s > 0.0 &&
		config.pitch_rate_command_integral_gain_rad_inv >= 0.0 &&
		config.pitch_rate_command_anti_windup_s_inv >= 0.0 &&
		config.pitch_rate_command_integral_limit_effort > 0.0;
}

bool valid_longitudinal_protection(
	const Systems::LongitudinalControlConfig& config)
{
	return config.angle_of_attack_limited_normal_acceleration_g > 0.0 &&
		config.limit_buffer_bias_g >= 0.0 &&
		config.landing_pitch_rate_limit_rad_s > 0.0;
}

bool valid_longitudinal_law(
	const Systems::LongitudinalControlConfig& config)
{
	return finite_longitudinal_law(config) &&
		valid_normal_acceleration_law(config) &&
		valid_normal_acceleration_integrator(config) &&
		valid_pitch_rate_law(config) &&
		valid_longitudinal_protection(config);
}

bool valid_inner_rate_law(const Systems::InnerRateControlConfig& config)
{
	return Common::all_finite({ config.roll_proportional,
		config.roll_integral, config.yaw_proportional,
		config.yaw_integral, config.anti_windup_gain,
		config.integral_limit }) && config.roll_proportional > 0.0 &&
		config.roll_integral >= 0.0 &&
		config.yaw_proportional > 0.0 &&
		config.yaw_integral >= 0.0 &&
		config.anti_windup_gain >= 0.0 && config.integral_limit > 0.0;
}

bool valid_law_config(const Systems::FlightControlLawsConfig& config)
{
	const auto& mixer = config.surface_mixer;
	return valid_longitudinal_law(config.longitudinal) &&
		valid_inner_rate_law(config.inner_rate) &&
		Common::all_finite({ mixer.symmetric_stabilator_limit_rad,
			mixer.differential_flaperon_limit_rad,
			mixer.rudder_limit_rad }) &&
		mixer.symmetric_stabilator_limit_rad > 0.0 &&
		mixer.differential_flaperon_limit_rad > 0.0 &&
		mixer.rudder_limit_rad > 0.0;
}

bool valid_output_config(const Systems::FlightControlOutputConfig& config)
{
	return Common::all_finite({ config.selection_transition_time_s,
		config.tracking_tolerance_rad }) &&
		config.selection_transition_time_s > 0.0 &&
		config.tracking_tolerance_rad > 0.0;
}

bool valid_diagnostics_config(
	const Systems::FlightControlDiagnosticsConfig& config)
{
	return Common::all_finite({ config.control_authority_persistence_s,
		config.minimum_alpha_recovery_rate_rad_s }) &&
		config.control_authority_persistence_s > 0.0 &&
		config.minimum_alpha_recovery_rate_rad_s >= 0.0;
}

bool valid_support_config(
	const Core::Systems::FlightControlComputerConfig& config)
{
	const auto& guidance = config.guidance_coordination;
	const auto& development = config.development;
	return Common::all_finite({ guidance.pitch_error_to_rate_gain_s_inv,
		guidance.vertical_speed_error_to_acceleration_gain_s_inv,
		guidance.bank_error_to_roll_rate_gain_s_inv,
		guidance.coordinated_turn_minimum_speed_mps,
		development.g_limiter_override_margin_g }) &&
		guidance.pitch_error_to_rate_gain_s_inv > 0.0 &&
		guidance.vertical_speed_error_to_acceleration_gain_s_inv > 0.0 &&
		guidance.bank_error_to_roll_rate_gain_s_inv > 0.0 &&
		guidance.coordinated_turn_minimum_speed_mps > 0.0 &&
		development.g_limiter_override_margin_g > 0.0;
}
}

namespace Core
{
namespace Systems
{
void validate_flight_control_computer_config(
	const FlightControlComputerConfig& config)
{
	validate_automatic_flight_control_config(
		config.automatic_flight_control);
	if (!valid_input_config(config.input_signal_management) ||
		!valid_mode_and_gain(config.mode_and_gain) ||
		!valid_law_config(config.flight_control_laws) ||
		!valid_output_config(config.flight_control_output) ||
		!valid_diagnostics_config(config.diagnostics) ||
		!valid_support_config(config) ||
		!coherent_guidance_envelope(config) ||
		!coherent_alpha_control(config))
	{
		throw std::invalid_argument(
			"FlightControlComputerConfig is incomplete or incoherent.");
	}
	(void)::Systems::make_maneuver_envelope(
		{ config.mode_and_gain, config.mode_and_gain.cat1,
			0.0, false, 0.0, false });
	(void)::Systems::make_maneuver_envelope(
		{ config.mode_and_gain, config.mode_and_gain.cat3,
			0.0, false, 0.0, true });
}

const FlightControlComputerConfig& fck1c_flight_control_computer_config()
{
	static const FlightControlComputerConfig config = make_fck1c_config();
	return config;
}
}
}
