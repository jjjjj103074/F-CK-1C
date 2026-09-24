#pragma once

namespace Systems
{
/// @brief 輸入訊號濾波與飛行狀態估測的固定參數。
struct InputSignalManagementConfig
{
	double signal_filter_time_constant_s = 0.06;  // 一般觀測與駕駛輸入的一階濾波時間常數，單位秒。
	double dynamic_pressure_filter_time_constant_s = 0.18;  // 動壓濾波時間常數，單位秒。
	double normal_acceleration_filter_time_constant_s = 0.26;  // 法向加速度濾波時間常數，單位秒。
};

/// @brief 驗證輸入訊號模組自己的固定參數。
/// @param config 要檢查的輸入訊號設定。
/// @throws std::invalid_argument 參數不是有限值或時間常數不為正值時擲出。
void validate_input_signal_management_config(
	const InputSignalManagementConfig& config);
}
