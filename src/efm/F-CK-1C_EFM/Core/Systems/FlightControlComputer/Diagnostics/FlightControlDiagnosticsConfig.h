#pragma once

#include "Common/Units.h"

namespace Systems
{
/// @brief 診斷狀態的成立條件；只影響觀測結果，不改變控制律。
struct FlightControlDiagnosticsConfig
{
	double control_authority_persistence_s = 0.5;  // 控制能力不足狀態成立前的持續時間。
	double minimum_alpha_recovery_rate_rad_s = Common::rad(0.5);  // 視為迎角正在恢復的最小下降速率。
};

/// @brief 驗證診斷模組自己的成立門檻。
/// @param config 要檢查的診斷設定。
/// @throws std::invalid_argument 持續時間或恢復率無效時擲出。
void validate_flight_control_diagnostics_config(
	const FlightControlDiagnosticsConfig& config);
}
