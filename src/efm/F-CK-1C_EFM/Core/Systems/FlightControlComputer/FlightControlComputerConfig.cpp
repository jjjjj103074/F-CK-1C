#include "FlightControlComputerConfig.h"
#include "Autopilot/AutomaticFlightControl.h"
#include "ControlLaws/ConfigurationAndMode.h"

#include "../../../Common/ConfigValidation.h"

#include <algorithm>
#include <stdexcept>

namespace
{
Core::Systems::FlightControlComputerConfig make_fck1c_config()
{
	Core::Systems::FlightControlComputerConfig config;
	config.automatic_flight_control =
		Core::Systems::fck1c_automatic_flight_control_config(
			config.control_laws);
	config.mach_table = { 0.0, 0.4, 0.6, 0.8, 0.9, 1.5 };
	config.alpha_limit_deg = { 20.0, 20.0, 20.0, 18.0, 15.0, 10.0 };
	return config;
}

bool valid_cat_hold_parameters(const Systems::FBWCatParams& config)
{
	return config.deadband >= 0.0 && config.deadband < 1.0 &&
		config.hold_engage_time >= 0.0 && config.hold_phi_kp >= 0.0 &&
		config.hold_theta_kp >= 0.0 && config.hold_p_cmd_max > 0.0 &&
		config.hold_q_cmd_max > 0.0 && config.qbar_min_hold >= 0.0 &&
		config.sat_time >= 0.0;
}

bool valid_cat_shape_parameters(const Systems::FBWCatParams& config)
{
	return config.hold_cmd_ratio_limit > 0.0 &&
		config.hold_cmd_ratio_limit <= 1.0 &&
		config.hold_decay_tau > 0.0 && config.command_shape_tau > 0.0 &&
		config.command_shape_rate > 0.0 && config.stick_expo >= 0.0 &&
		config.alpha_hold_degrade_deg > 0.0;
}

bool valid_cat_command_limits(const Systems::FBWCatParams& config)
{
	return config.p_cmd_max > 0.0 && config.r_cmd_max > 0.0 &&
		config.p_rate_limit > 0.0 &&
		config.q_rate_limit > 0.0 && config.r_rate_limit > 0.0;
}

bool valid_cat_envelope(const Systems::FBWCatParams& config)
{
	return config.aoa_soft_deg > 0.0 &&
		config.aoa_hard_deg > config.aoa_soft_deg &&
		config.g_soft > 0.0 && config.g_hard > config.g_soft &&
		config.yaw_damper_beta >= 0.0 && config.yaw_damper_r >= 0.0;
}

bool valid_cat_parameters(const Systems::FBWCatParams& config)
{
	const bool finite = Common::all_finite({
		config.deadband, config.hold_engage_time, config.hold_phi_kp,
		config.hold_theta_kp, config.hold_p_cmd_max,
		config.hold_q_cmd_max, config.alpha_hold_degrade_deg,
		config.qbar_min_hold, config.sat_time, config.hold_cmd_ratio_limit,
		config.hold_decay_tau, config.command_shape_tau,
		config.command_shape_rate, config.stick_expo, config.p_cmd_max,
		config.r_cmd_max, config.aoa_soft_deg,
		config.aoa_hard_deg, config.g_soft, config.g_hard,
		config.p_rate_limit, config.q_rate_limit, config.r_rate_limit,
		config.yaw_damper_beta, config.yaw_damper_r
	});
	return finite && valid_cat_hold_parameters(config) &&
		valid_cat_shape_parameters(config) &&
		valid_cat_command_limits(config) && valid_cat_envelope(config);
}

bool valid_gain_schedule(const Systems::FBWControllerConfig& config)
{
	double previous_qbar = -1.0;
	for (const Systems::FBWGainSchedulePoint& point : config.gain_schedule)
	{
		const bool finite = Common::all_finite({
			point.qbar, point.cmd_gain, point.hold_gain,
			point.damping_gain, point.limiter_gain
		});
		if (!finite || point.qbar <= previous_qbar ||
			point.cmd_gain <= 0.0 || point.hold_gain <= 0.0 ||
			point.damping_gain <= 0.0 || point.limiter_gain <= 0.0)
		{
			return false;
		}
		previous_qbar = point.qbar;
	}
	return true;
}

bool valid_signal_time_constants(
	const Systems::FBWControllerConfig& config)
{
	return config.mode_switch_tau > 0.0 &&
		config.signal_filter_tau > 0.0 && config.qbar_filter_tau > 0.0 &&
		config.nz_filter_tau > 0.0 && config.pitch_ref_tau > 0.0;
}

bool valid_controller_outer_limits(
	const Systems::FBWControllerConfig& config)
{
	return config.int_limit > 0.0 && config.outer_int_limit > 0.0 &&
		config.developer_g_limiter_override_margin_g > 0.0 &&
		config.pitch_ref_rate_deg_s > 0.0 &&
		config.nz_limit_gain_floor > 0.0 &&
		config.nz_limit_gain_floor <= 1.0 &&
		config.nz_limit_buffer_bias >= 0.0 &&
		config.q_cmd_land_max_deg > 0.0;
}

bool valid_direct_mode_config(const Systems::FBWDirectModeConfig& config)
{
	const bool finite = Common::all_finite({
		config.elevator_command_step_normalized,
		config.aileron_command_step_normalized,
		config.rudder_command_step_normalized
	});
	return finite && config.elevator_command_step_normalized > 0.0 &&
		config.elevator_command_step_normalized <= 1.0 &&
		config.aileron_command_step_normalized > 0.0 &&
		config.aileron_command_step_normalized <= 1.0 &&
		config.rudder_command_step_normalized > 0.0 &&
		config.rudder_command_step_normalized <= 1.0;
}

bool valid_normal_acceleration_config(
	const Systems::FBWNormalAccelerationConfig& config)
{
	const bool finite = Common::all_finite({
		config.positive_buffer_minimum_g, config.negative_soft_minimum_g,
		config.negative_soft_ratio, config.outer_kp_cat1,
		config.outer_kp_cat3, config.outer_ki_cat1,
		config.outer_ki_cat3, config.limit_range_minimum_g,
		config.minimum_aoa_soft_limit_deg,
		config.alpha_protection_rate_gain_s_inv,
		config.g_limit_activation_tolerance_g
	});
	return finite && config.positive_buffer_minimum_g > 0.0 &&
		config.negative_soft_minimum_g > 0.0 &&
		config.negative_soft_ratio > 0.0 &&
		config.negative_soft_ratio <= 1.0 && config.outer_kp_cat1 > 0.0 &&
		config.outer_kp_cat3 > 0.0 && config.outer_ki_cat1 > 0.0 &&
		config.outer_ki_cat3 > 0.0 && config.limit_range_minimum_g > 0.0 &&
		config.minimum_aoa_soft_limit_deg > 0.0 &&
		config.alpha_protection_rate_gain_s_inv > 0.0 &&
		config.g_limit_activation_tolerance_g > 0.0;
}

bool valid_hold_degrade_config(const Systems::FBWHoldDegradeConfig& config)
{
	const bool finite = Common::all_finite({
		config.alpha_limit_ratio, config.gain_zero_threshold,
		config.actuator_timer_maximum_s
	});
	return finite && config.alpha_limit_ratio > 0.0 &&
		config.alpha_limit_ratio <= 1.0 &&
		config.gain_zero_threshold > 0.0 &&
		config.gain_zero_threshold <= 1.0 &&
		config.actuator_timer_maximum_s > 0.0;
}

bool valid_controller_parameters(const Systems::FBWControllerConfig& config)
{
	const bool finite = Common::all_finite({
		config.mode_switch_tau, config.signal_filter_tau,
		config.qbar_filter_tau, config.kp_p, config.ki_p, config.kp_q,
		config.ki_q, config.kp_r, config.ki_r, config.aw_gain,
		config.int_limit, config.outer_aw_gain, config.outer_int_limit,
		config.nz_filter_tau,
		config.pitch_ref_tau, config.pitch_ref_rate_deg_s,
		config.nz_limit_gain_floor, config.nz_limit_buffer_bias,
		config.q_cmd_land_max_deg,
		config.developer_g_limiter_override_margin_g,
		config.pitch_attitude_error_to_rate_gain,
		config.vertical_speed_error_to_acceleration_gain,
		config.bank_angle_error_to_roll_rate_gain,
		config.coordinated_turn_minimum_speed_mps
	});
	const bool valid_guidance_coordination =
		config.pitch_attitude_error_to_rate_gain > 0.0 &&
		config.vertical_speed_error_to_acceleration_gain > 0.0 &&
		config.bank_angle_error_to_roll_rate_gain > 0.0 &&
		config.coordinated_turn_minimum_speed_mps > 0.0;
	return finite && valid_signal_time_constants(config) &&
		valid_controller_outer_limits(config) &&
		valid_direct_mode_config(config.direct_mode) &&
		valid_normal_acceleration_config(config.normal_acceleration) &&
		valid_hold_degrade_config(config.hold_degrade) &&
		valid_guidance_coordination;
}

bool coherent_guidance_envelope(
	const Core::Systems::FlightControlComputerConfig& config)
{
	return config.automatic_flight_control.bank_limit_rad ==
			config.control_laws.guidance_bank_limit_rad &&
		config.automatic_flight_control.roll_reference_rate_rad_s ==
			config.control_laws.guidance_roll_rate_limit_rad_s;
}
}

namespace Core
{
namespace Systems
{
void validate_flight_control_computer_config(
	const FlightControlComputerConfig& config)
{
	const bool valid_envelope =
		Common::finite_strictly_increasing(config.mach_table) &&
		config.alpha_limit_deg.size() == config.mach_table.size() &&
		Common::all_finite(config.alpha_limit_deg) &&
		std::all_of(
			config.alpha_limit_deg.begin(),
			config.alpha_limit_deg.end(),
			[](double value) { return value > 0.0; });
	const bool valid_control_laws =
		valid_cat_parameters(config.control_laws.cat1) &&
		valid_cat_parameters(config.control_laws.cat3) &&
		valid_gain_schedule(config.control_laws) &&
		valid_controller_parameters(config.control_laws);
	validate_automatic_flight_control_config(
		config.automatic_flight_control);
	if (!valid_envelope || !valid_control_laws ||
		!coherent_guidance_envelope(config))
	{
		throw std::invalid_argument(
			"FlightControlComputerConfig requires valid control laws and "
			"a complete flight envelope.");
	}
	const double minimum_alpha_limit_deg = *std::min_element(
		config.alpha_limit_deg.begin(), config.alpha_limit_deg.end());
	(void)::Systems::make_maneuver_envelope(
		config.control_laws,
		{ config.control_laws.cat1, minimum_alpha_limit_deg, false });
	(void)::Systems::make_maneuver_envelope(
		config.control_laws,
		{ config.control_laws.cat3, minimum_alpha_limit_deg, false });
}

const FlightControlComputerConfig& fck1c_flight_control_computer_config()
{
	static const FlightControlComputerConfig config = make_fck1c_config();
	return config;
}
}
}
