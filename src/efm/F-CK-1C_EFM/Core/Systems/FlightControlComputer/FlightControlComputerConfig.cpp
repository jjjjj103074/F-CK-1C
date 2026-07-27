#include "FlightControlComputerConfig.h"

#include "../../../Common/ConfigValidation.h"

#include <algorithm>
#include <stdexcept>

namespace
{
Core::Systems::FlightControlComputerConfig make_fck1c_config()
{
	Core::Systems::FlightControlComputerConfig config;
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
	return config.p_cmd_max > 0.0 && config.q_cmd_max > 0.0 &&
		config.r_cmd_max > 0.0 && config.p_rate_limit > 0.0 &&
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
		config.q_cmd_max, config.r_cmd_max, config.aoa_soft_deg,
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

bool valid_controller_regions(const Systems::FBWControllerConfig& config)
{
	return config.region_high_kts > config.region_low_kts &&
		config.region_approach_kts > config.region_min_kts &&
		config.region_alpha2_deg > config.region_alpha1_deg;
}

bool valid_signal_time_constants(
	const Systems::FBWControllerConfig& config)
{
	return config.mode_switch_tau > 0.0 &&
		config.signal_filter_tau > 0.0 && config.qbar_filter_tau > 0.0 &&
		config.alpha_trim_tau > 0.0 && config.nz_trim_tau > 0.0 &&
		config.nz_filter_tau > 0.0 && config.pitch_ref_tau > 0.0;
}

bool valid_actuator_time_constants(
	const Systems::FBWControllerConfig& config)
{
	return config.ail_lag_tau > 0.0 && config.ele_lag_tau > 0.0 &&
		config.rud_lag_tau > 0.0;
}

bool valid_controller_outer_limits(
	const Systems::FBWControllerConfig& config)
{
	return config.int_limit > 0.0 && config.outer_int_limit > 0.0 &&
		config.pitch_ref_rate_deg_s > 0.0 &&
		config.nz_limit_gain_floor > 0.0 &&
		config.nz_limit_gain_floor <= 1.0 &&
		config.nz_limit_buffer_bias >= 0.0 &&
		config.alpha_cmd_per_stick_deg > 0.0 &&
		config.q_cmd_land_max_deg > 0.0;
}

bool valid_actuator_limits(const Systems::FBWControllerConfig& config)
{
	return config.ail_limit_deg > 0.0 && config.ele_limit_deg > 0.0 &&
		config.rud_limit_deg > 0.0 && config.ail_rate_deg_s > 0.0 &&
		config.ele_rate_deg_s > 0.0 && config.rud_rate_deg_s > 0.0;
}

bool valid_controller_parameters(const Systems::FBWControllerConfig& config)
{
	const bool finite = Common::all_finite({
		config.mode_switch_tau, config.signal_filter_tau,
		config.qbar_filter_tau, config.kp_p, config.ki_p, config.kp_q,
		config.ki_q, config.kp_r, config.ki_r, config.aw_gain,
		config.int_limit, config.outer_aw_gain, config.outer_int_limit,
		config.alpha_trim_tau, config.nz_trim_tau, config.nz_filter_tau,
		config.pitch_ref_tau, config.pitch_ref_rate_deg_s,
		config.nz_limit_gain_floor, config.nz_limit_buffer_bias,
		config.region_low_kts, config.region_high_kts,
		config.region_approach_kts, config.region_min_kts,
		config.region_alpha1_deg, config.region_alpha2_deg,
		config.alpha_cmd_per_stick_deg, config.q_cmd_land_max_deg,
		config.ail_limit_deg, config.ele_limit_deg, config.rud_limit_deg,
		config.ail_rate_deg_s, config.ele_rate_deg_s, config.rud_rate_deg_s,
		config.ail_lag_tau, config.ele_lag_tau, config.rud_lag_tau
	});
	return finite && valid_controller_regions(config) &&
		valid_signal_time_constants(config) &&
		valid_actuator_time_constants(config) &&
		valid_controller_outer_limits(config) &&
		valid_actuator_limits(config);
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
	if (!valid_envelope || !valid_control_laws)
	{
		throw std::invalid_argument(
			"FlightControlComputerConfig requires valid control laws and "
			"a complete flight envelope.");
	}
}

const FlightControlComputerConfig& fck1c_flight_control_computer_config()
{
	static const FlightControlComputerConfig config = make_fck1c_config();
	return config;
}
}
}
