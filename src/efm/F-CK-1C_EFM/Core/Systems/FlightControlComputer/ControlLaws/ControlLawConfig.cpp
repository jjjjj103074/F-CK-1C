#include "ControlLawConfig.h"

#include "Common/ConfigValidation.h"

#include <stdexcept>

namespace
{
/// @brief 檢查縱向控制律使用的每個浮點設定是否為有限值。
/// @param config 要檢查的縱向控制律設定。
/// @return 沒有 NaN 或無限值時為 true。
bool finite_longitudinal_law(const Systems::LongitudinalControlConfig& config)
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
		config.limit_buffer_bias_g, config.landing_pitch_rate_limit_rad_s });
}

/// @brief 檢查法向加速度控制的比例與柔化參數。
/// @param config 要檢查的縱向控制律設定。
/// @return 比例增益與過載緩衝範圍有效時為 true。
bool valid_normal_acceleration_law(
	const Systems::LongitudinalControlConfig& config)
{
	return config.positive_buffer_minimum_g > 0.0 &&
		config.negative_soft_minimum_g > 0.0 &&
		config.negative_soft_ratio > 0.0 && config.negative_soft_ratio <= 1.0 &&
		config.normal_acceleration_proportional_cat1 > 0.0 &&
		config.normal_acceleration_proportional_cat3 > 0.0;
}

/// @brief 檢查法向加速度積分器與反飽和參數。
/// @param config 要檢查的縱向控制律設定。
/// @return 積分增益非負且積分限制為正值時為 true。
bool valid_normal_acceleration_integrator(
	const Systems::LongitudinalControlConfig& config)
{
	return config.normal_acceleration_integral_cat1_s_inv >= 0.0 &&
		config.normal_acceleration_integral_cat3_s_inv >= 0.0 &&
		config.normal_acceleration_anti_windup_s_inv >= 0.0 &&
		config.normal_acceleration_integral_limit_effort > 0.0;
}

/// @brief 檢查俯仰角速度迴路的增益與積分器參數。
/// @param config 要檢查的縱向控制律設定。
/// @return 回授、比例、積分與反飽和參數有效時為 true。
bool valid_pitch_rate_law(const Systems::LongitudinalControlConfig& config)
{
	return config.pitch_rate_washout_time_constant_s > 0.0 &&
		config.pitch_rate_feedback_gain_s > 0.0 &&
		config.angle_of_attack_stability_gain_rad_inv > 0.0 &&
		config.pitch_rate_command_proportional_s > 0.0 &&
		config.pitch_rate_command_integral_gain_rad_inv >= 0.0 &&
		config.pitch_rate_command_anti_windup_s_inv >= 0.0 &&
		config.pitch_rate_command_integral_limit_effort > 0.0;
}

/// @brief 檢查迎角保護與起落架放下時的限制參數。
/// @param config 要檢查的縱向控制律設定。
/// @return 保護目標及角速度限制有效時為 true。
bool valid_longitudinal_protection(
	const Systems::LongitudinalControlConfig& config)
{
	return config.angle_of_attack_limited_normal_acceleration_g > 0.0 &&
		config.limit_buffer_bias_g >= 0.0 &&
		config.landing_pitch_rate_limit_rad_s > 0.0;
}

/// @brief 彙整縱向控制律所有局部驗證。
/// @param config 要檢查的縱向控制律設定。
/// @return 每一組縱向規則都成立時為 true。
bool valid_longitudinal_law(const Systems::LongitudinalControlConfig& config)
{
	return finite_longitudinal_law(config) &&
		valid_normal_acceleration_law(config) &&
		valid_normal_acceleration_integrator(config) &&
		valid_pitch_rate_law(config) &&
		valid_longitudinal_protection(config);
}

/// @brief 檢查橫向與方向角速度內迴路設定。
/// @param config 要檢查的內迴路設定。
/// @return 增益、反飽和與積分限制有效時為 true。
bool valid_inner_rate_law(const Systems::InnerRateControlConfig& config)
{
	return Common::all_finite({ config.roll_proportional,
		config.roll_integral, config.yaw_proportional, config.yaw_integral,
		config.anti_windup_gain, config.integral_limit }) &&
		config.roll_proportional > 0.0 && config.roll_integral >= 0.0 &&
		config.yaw_proportional > 0.0 && config.yaw_integral >= 0.0 &&
		config.anti_windup_gain >= 0.0 && config.integral_limit > 0.0;
}

/// @brief 檢查控制面混合器的電子偏轉限制。
/// @param config 要檢查的控制面混合設定。
/// @return 三個控制面限制都是有限正值時為 true。
bool valid_surface_mixer(const Systems::SurfaceCommandMixerConfig& config)
{
	return Common::all_finite({ config.symmetric_stabilator_limit_rad,
		config.differential_flaperon_limit_rad, config.rudder_limit_rad }) &&
		config.symmetric_stabilator_limit_rad > 0.0 &&
		config.differential_flaperon_limit_rad > 0.0 &&
		config.rudder_limit_rad > 0.0;
}
}

namespace Systems
{
void validate_flight_control_laws_config(
	const FlightControlLawsConfig& config)
{
	if (!valid_longitudinal_law(config.longitudinal) ||
		!valid_inner_rate_law(config.inner_rate) ||
		!valid_surface_mixer(config.surface_mixer))
	{
		throw std::invalid_argument(
			"Invalid FLCC control-law configuration.");
	}
}
}
