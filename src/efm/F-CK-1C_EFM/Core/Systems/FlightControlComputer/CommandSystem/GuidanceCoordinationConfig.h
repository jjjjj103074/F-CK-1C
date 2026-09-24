#pragma once

namespace Systems
{
/// @brief 將飛行目標轉換並協調成三軸機動要求的固定參數。
struct GuidanceCoordinationConfig
{
	double pitch_error_to_rate_gain_s_inv = 2.5;  // 俯仰姿態誤差轉為俯仰角速度要求的增益。
	double vertical_speed_error_to_acceleration_gain_s_inv = 0.25;  // 垂直速度誤差轉為加速度要求的增益。
	double bank_error_to_roll_rate_gain_s_inv = 2.0;  // 傾斜角誤差轉為滾轉角速度要求的增益。
	double coordinated_turn_minimum_speed_mps = 30.0;  // 啟用協調轉彎計算的最低真空速。
};

/// @brief 驗證三軸指引協調模組自己的增益與最低速度。
/// @param config 要檢查的三軸指引協調設定。
/// @throws std::invalid_argument 任一設定不是有限正值時擲出。
void validate_guidance_coordination_config(
	const GuidanceCoordinationConfig& config);
}
