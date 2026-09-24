#pragma once

#include "Common/Units.h"

namespace Systems
{
/// @brief 縱向控制律的固定增益、限制與回授參數。
struct LongitudinalControlConfig
{
	double positive_buffer_minimum_g = 0.25;  // 正向過載限制前保留的最小緩衝。
	double negative_soft_minimum_g = 1.0;  // 負向過載柔化區的最小寬度。
	double negative_soft_ratio = 0.65;  // 進入負向柔化區後保留的指令比例。
	// 控制拓撲源自參考資料；數值依目前 EFM plant 調校。
	double normal_acceleration_proportional_cat1 = 0.30;  // CAT I 法向加速度比例增益。
	double normal_acceleration_proportional_cat3 = 0.22;  // CAT III 法向加速度比例增益。
	double normal_acceleration_integral_cat1_s_inv = 0.16;  // CAT I 法向加速度積分增益。
	double normal_acceleration_integral_cat3_s_inv = 0.10;  // CAT III 法向加速度積分增益。
	double normal_acceleration_anti_windup_s_inv = 1.20;  // 法向加速度積分器的反飽和增益。
	double normal_acceleration_integral_limit_effort = 0.65;  // 法向加速度積分修正量上限。
	double pitch_rate_washout_time_constant_s = 0.35;  // 俯仰角速度洗出濾波時間常數。
	double pitch_rate_feedback_gain_s = 0.85;  // 俯仰角速度回授增益。
	double angle_of_attack_stability_gain_rad_inv = 1.20;  // 迎角穩定回授增益。
	double pitch_rate_command_proportional_s = 0.88;  // 俯仰角速度指令比例增益。
	double pitch_rate_command_integral_gain_rad_inv = 0.48;  // 俯仰角速度誤差積分增益。
	double pitch_rate_command_anti_windup_s_inv = 1.20;  // 俯仰角速度積分器的反飽和增益。
	double pitch_rate_command_integral_limit_effort = 1.20;  // 俯仰角速度積分修正量上限。
	double angle_of_attack_limited_normal_acceleration_g = 1.0;  // 迎角硬限制時採用的法向過載目標。
	double limit_buffer_bias_g = 0.15;  // 包線限制器額外保留的過載緩衝。
	double landing_pitch_rate_limit_rad_s = Common::rad(50.0);  // 起落架放下時的俯仰角速度上限。
};

/// @brief 橫向與方向內迴路的固定增益及積分限制。
struct InnerRateControlConfig
{
	double roll_proportional = 0.55;  // 滾轉角速度比例增益。
	double roll_integral = 0.35;  // 滾轉角速度積分增益。
	double yaw_proportional = 0.65;  // 偏航角速度比例增益。
	double yaw_integral = 0.25;  // 偏航角速度積分增益。
	double anti_windup_gain = 1.20;  // 橫向與方向積分器的反飽和增益。
	double integral_limit = 1.20;  // 橫向與方向積分修正量上限。
};

/// @brief 三軸控制努力轉為電子控制面需求時的最大偏轉。
struct SurfaceCommandMixerConfig
{
	double symmetric_stabilator_limit_rad = Common::rad(25.0);  // 對稱水平尾翼電子需求上限。
	double differential_flaperon_limit_rad = Common::rad(22.0);  // 差動襟副翼電子需求上限。
	double rudder_limit_rad = Common::rad(30.0);  // 方向舵電子需求上限。
};

/// @brief FLCC 三軸控制律及控制面混合設定。
struct FlightControlLawsConfig
{
	LongitudinalControlConfig longitudinal;  // 縱向控制律設定。
	InnerRateControlConfig inner_rate;  // 橫向與方向內迴路設定。
	SurfaceCommandMixerConfig surface_mixer;  // 控制努力至控制面需求的混合設定。
};

/// @brief 驗證控制律模組自己的增益、積分器與控制面限制。
/// @param config 要檢查的三軸控制律設定。
/// @throws std::invalid_argument 任一控制律設定無效時擲出。
void validate_flight_control_laws_config(
	const FlightControlLawsConfig& config);
}
