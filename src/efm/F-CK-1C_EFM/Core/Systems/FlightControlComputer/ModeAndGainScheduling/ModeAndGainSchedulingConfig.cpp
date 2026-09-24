#include "ModeAndGainSchedulingConfig.h"

#include "ModeAndGainScheduling.h"
#include "Common/ConfigValidation.h"
#include "Common/Units.h"

#include <algorithm>
#include <stdexcept>

namespace
{
/// @brief 檢查一組駕駛輸入塑形參數。
/// @param config 要檢查的塑形參數。
/// @return 所有值與範圍都可供運算時為 true。
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

/// @brief 檢查單一掛載類別的機動包線。
/// @param envelope 要檢查的角速度與過載界線。
/// @return 上下限順序與所有正值限制都正確時為 true。
bool valid_envelope(const Systems::ManeuverEnvelopeSchedule& envelope)
{
	return Common::all_finite({ envelope.maximum_roll_command_rad_s,
		envelope.maximum_yaw_command_rad_s,
		envelope.soft_positive_load_factor_g,
		envelope.hard_positive_load_factor_g,
		envelope.roll_rate_limit_rad_s, envelope.pitch_rate_limit_rad_s,
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

/// @brief 檢查單一掛載類別的塑形、包線與方向控制排程。
/// @param schedule 要檢查的完整掛載排程。
/// @return 三個部分都有效時為 true。
bool valid_stores_schedule(const Systems::StoresControlLawSchedule& schedule)
{
	const auto& directional = schedule.directional;
	return valid_pilot_input(schedule.pilot_input) &&
		valid_envelope(schedule.envelope) &&
		Common::all_finite({ directional.sideslip_damping_s_inv,
			directional.yaw_rate_damping }) &&
		directional.sideslip_damping_s_inv >= 0.0 &&
		directional.yaw_rate_damping >= 0.0;
}

/// @brief 檢查動壓增益表的數值與排序。
/// @param config 含固定長度增益表的模式設定。
/// @return 動壓嚴格遞增且所有增益為正值時為 true。
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

/// @brief 檢查馬赫數與迎角限制兩張對應資料表。
/// @param config 含馬赫數軸與迎角限制值的模式設定。
/// @return 兩表長度相同、馬赫數遞增且迎角限制為正值時為 true。
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

/// @brief 檢查不屬於資料表的模式與包線純量。
/// @param config 要檢查的模式設定。
/// @return 所有純量有限且必要的時間、角度與餘裕為正值時為 true。
bool valid_scalars(const Systems::ModeAndGainSchedulingConfig& config)
{
	return Common::all_finite({ config.stores_transition_time_constant_s,
		config.guidance_bank_limit_rad,
		config.guidance_roll_rate_limit_rad_s,
		config.guidance_minimum_load_factor_g,
		config.guidance_maximum_load_factor_g, config.hard_bank_limit_rad,
		config.hard_minimum_load_factor_g,
		config.cruise_angle_of_attack_blend_start_rad,
		config.landing_angle_of_attack_blend_start_rad,
		config.landing_angle_of_attack_limit_rad,
		config.g_limiter_override.margin_g }) &&
		config.stores_transition_time_constant_s > 0.0 &&
		config.cruise_angle_of_attack_blend_start_rad > 0.0 &&
		config.landing_angle_of_attack_blend_start_rad > 0.0 &&
		config.landing_angle_of_attack_limit_rad >
			config.landing_angle_of_attack_blend_start_rad &&
		config.g_limiter_override.margin_g > 0.0;
}

/// @brief 確認巡航迎角限制都高於保護開始位置。
/// @param config 含迎角限制表與混合起點的模式設定。
/// @return 每個迎角硬限制都留有保護混合區時為 true。
bool cruise_limits_exceed_blend_start(
	const Systems::ModeAndGainSchedulingConfig& config)
{
	return std::all_of(config.angle_of_attack_limit_rad.begin(),
		config.angle_of_attack_limit_rad.end(),
		[&config](double limit_rad)
		{
			return limit_rad > config.cruise_angle_of_attack_blend_start_rad;
		});
}
}

namespace Systems
{
ModeAndGainSchedulingConfig make_fck1c_mode_and_gain_scheduling_config()
{
	ModeAndGainSchedulingConfig config;
	config.cat1 = {
		{ 0.03, 0.05, 9.5, 0.10 },
		{ Common::rad(190.0), Common::rad(80.0), 6.4, 8.8,
			Common::rad(220.0), Common::rad(170.0), Common::rad(95.0) },
		{ 0.90, 0.60 }
	};
	config.cat3 = {
		{ 0.03, 0.10, 5.5, 0.20 },
		{ Common::rad(140.0), Common::rad(60.0), 5.8, 7.6,
			Common::rad(170.0), Common::rad(130.0), Common::rad(75.0) },
		{ 1.10, 0.80 }
	};
	config.gain_schedule = {{
		{ 1500.0, 1.15, 1.15, 0.82 },
		{ 5000.0, 1.05, 1.00, 0.95 },
		{ 15000.0, 0.90, 0.90, 1.00 },
		{ 35000.0, 0.75, 0.80, 0.90 }
	}};
	config.stores_transition_time_constant_s = 0.45;
	config.guidance_bank_limit_rad = Common::rad(30.0);
	config.guidance_roll_rate_limit_rad_s = Common::rad(20.0);
	config.guidance_minimum_load_factor_g = 0.5;
	config.guidance_maximum_load_factor_g = 2.0;
	config.hard_bank_limit_rad = Common::rad(60.0);
	config.hard_minimum_load_factor_g = -2.5;
	config.angle_of_attack_mach = { 0.0, 0.85, 0.95, 1.5 };
	config.angle_of_attack_limit_rad = {
		Common::rad(29.0), Common::rad(29.0),
		Common::rad(26.0), Common::rad(26.0)
	};
	config.cruise_angle_of_attack_blend_start_rad = Common::rad(19.0);
	config.landing_angle_of_attack_blend_start_rad = Common::rad(10.0);
	config.landing_angle_of_attack_limit_rad = Common::rad(16.0);
	return config;
}

void validate_mode_and_gain_scheduling_config(
	const ModeAndGainSchedulingConfig& config)
{
	const bool valid = valid_stores_schedule(config.cat1) &&
		valid_stores_schedule(config.cat3) && valid_gain_schedule(config) &&
		valid_angle_of_attack_schedule(config) && valid_scalars(config) &&
		cruise_limits_exceed_blend_start(config);
	if (!valid)
	{
		throw std::invalid_argument(
			"Invalid FLCC mode or gain configuration.");
	}

	// 用正式的包線組合函式再檢查一次跨欄位順序，避免驗證規則與執行規則分歧。
	(void)make_maneuver_envelope({ config, config.cat1, 0.0, false, 0.0, false });
	(void)make_maneuver_envelope({ config, config.cat3, 0.0, false, 0.0, true });
}
}
