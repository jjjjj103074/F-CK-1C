#pragma once

#include "ExperimentalAutoThrottleAssistConfig.h"
#include "../Contracts/FlightControlReferences.h"

// Public configuration contract; AFCS behavior remains CommandSystem internal.

namespace Core::Systems
{
/// @brief 自動飛行從共用飛行包線取得的橫向限制。
/// 這些值只有一個設定來源，由 Executive 在建構時投影給自動飛行模組。
struct AutomaticFlightGuidanceLimits
{
	double bank_limit_rad = 0.0;  // 自動飛行可要求的最大傾斜角。
	double roll_reference_rate_rad_s = 0.0;  // 傾斜角參考值的最大變化率。
};

/// @brief 自動飛行模式、參考值產生、監測與實驗功能的固定參數。
struct AutomaticFlightControlConfig
{
	double minimum_ias_mps = 0.0;  // 允許接通自動飛行的最低指示空速。
	double engage_roll_limit_rad = 0.0;  // 允許接通時的最大滾轉姿態絕對值。
	double engage_pitch_limit_rad = 0.0;  // 允許接通時的最大俯仰姿態絕對值。
	double pitch_reference_rate_rad_s = 0.0;  // 俯仰姿態參考值的最大變化率。
	double vertical_reference_acceleration_ft_s2 = 0.0;  // 垂直速度參考值的最大加速度。
	double altitude_fine_band_ft = 0.0;  // 使用精細高度修正的誤差範圍。
	double altitude_hold_band_ft = 0.0;  // 視為高度保持區域的誤差範圍。
	double altitude_capture_band_ft = 0.0;  // 開始捕獲目標高度的誤差範圍。
	double altitude_comfort_acceleration_ft_s2 = 0.0;  // 高度捕獲使用的舒適垂直加速度。
	double altitude_fine_gain_s_inv = 0.0;  // 精細高度修正增益。
	double altitude_hold_gain_s_inv = 0.0;  // 高度保持修正增益。
	double altitude_capture_gain_s_inv = 0.0;  // 高度捕獲修正增益。
	double altitude_approach_gain_s_inv = 0.0;  // 接近捕獲區域時的高度修正增益。
	double capture_minimum_vertical_speed_ft_s = 0.0;  // 捕獲高度時保留的最低垂直速度。
	double capture_comfort_distance_factor = 0.0;  // 捕獲距離中用於舒適減速的比例。
	double approach_comfort_distance_factor = 0.0;  // 接近距離中用於舒適減速的比例。
	double vertical_speed_limit_ft_s = 0.0;  // 自動飛行可要求的垂直速度上限。
	double vertical_speed_capture_taper_ft_s = 0.0;  // 進入捕獲時開始縮減垂直速度的區間。
	double pitch_stick_steering_threshold_normalized = 0.0;  // 進入俯仰搖桿操控的正規化門檻。
	double roll_stick_steering_threshold_normalized = 0.0;  // 進入滾轉搖桿操控的正規化門檻。
	int heading_select_step_deg = 0;  // 每次航向選擇指令改變的度數。
	double pitch_tracking_error_limit_rad = 0.0;  // 允許的俯仰追蹤誤差。
	double vertical_speed_tracking_error_limit_ft_s = 0.0;  // 允許的垂直速度追蹤誤差。
	double bank_tracking_error_limit_rad = 0.0;  // 允許的傾斜角追蹤誤差。
	double tracking_failure_persistence_s = 0.0;  // 追蹤誤差成立為故障前的持續時間。
	double actuator_saturation_persistence_s = 0.0;  // 致動器飽和成立為故障前的持續時間。
	double heading_kp = 0.0;  // 航向保持控制器的比例增益。
	double heading_ki = 0.0;  // 航向保持控制器的積分增益。
	double heading_error_integral_limit_deg_s = 0.0;  // 航向誤差積分狀態的限制。
	ExperimentalAutoThrottleAssistConfig experimental_auto_throttle;  // 實驗性自動油門的控制參數。
	bool experimental_auto_throttle_available = false;  // 是否開放非正式的自動油門輔助。
};

/// @brief 建立完整的 F-CK-1C 專案自動飛行設定。
/// @return 可放入 FLCC 設定草稿的獨立值物件。
AutomaticFlightControlConfig fck1c_automatic_flight_control_config();

/// @brief 驗證自動飛行模組自己的固定參數。
/// @param config 要檢查的自動飛行設定。
/// @throws std::invalid_argument 任一增益、門檻或範圍無效時擲出。
void validate_automatic_flight_control_config(
	const AutomaticFlightControlConfig& config);
}
