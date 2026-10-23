#pragma once

namespace Core::Systems::Mc
{
/// @brief 由 StartupAndRestart 寫入的生命週期分區；完整啟動狀態尚未定義。
struct LifecycleStatus
{
};

/// @brief 由 SystemMonitor 寫入的健康分區；監控結果及故障紀錄尚未定義。
struct HealthStatus
{
};

/// @brief 由 FailureManager 寫入的處置分區；降級、停用與恢復決策尚未定義。
struct FailureResponse
{
};

/// @brief MC 持有的三個狀態分區；不代表任何功能已可用，也不直接對外發布。
/// 元件只取得必要分區的參照，避免整份狀態表具有多個不受限制的寫入者。
struct SystemStatusTables
{
	LifecycleStatus lifecycle;
	HealthStatus health;
	FailureResponse failure_response;
};
}
