#pragma once

#include "Common/Units.h"

namespace Systems
{
/// @brief 電子控制律選擇、輸出切換及致動器追蹤設定。
struct FlightControlOutputConfig
{
	double selection_transition_time_s = 0.15;  // 控制律切換的無突變過渡時間，單位秒。
	double tracking_tolerance_rad = Common::rad(2.0);  // 命令與致動器位置的容許誤差。
	bool developer_direct_control_law = false;  // 是否使用開發用直接控制律輸出。
};

/// @brief 驗證電子輸出模組自己的切換與追蹤設定。
/// @param config 要檢查的電子輸出設定。
/// @throws std::invalid_argument 時間或角度設定無效時擲出。
void validate_flight_control_output_config(
	const FlightControlOutputConfig& config);
}
