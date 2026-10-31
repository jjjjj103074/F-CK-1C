#pragma once

namespace Core::Systems::Mc
{
/// @brief StartupAndRestart 持有參照的生命週期分區。
struct LifecycleStatus
{
};

/// @brief SystemMonitor 持有參照的健康分區。
struct HealthStatus
{
};

/// @brief FailureManager 持有參照的處置分區。
struct FailureResponse
{
};

/// @brief MC 持有的生命週期、健康與處置分區。
/// 各元件持有對應分區的參照。
struct SystemStatusTables
{
	LifecycleStatus lifecycle;
	HealthStatus health;
	FailureResponse failure_response;
};
}
