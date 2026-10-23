#pragma once

#include <chrono>

namespace Core::Systems::Mc
{
struct HealthStatus;
struct FailureResponse;

/// @brief MC 故障處置骨架；唯讀監控結果，只能寫入處置分區。
/// 不擁有資料庫，不自行註冊任務，也不直接重啟 Executive。
class FailureManager final
{
public:
	FailureManager(const HealthStatus& health, FailureResponse& response);
	/// @brief 接收一次排程呼叫；尚未執行停用、降級或恢復決策。
	void step(std::chrono::nanoseconds scheduled_time, double dt_s);

private:
	const HealthStatus& health_;
	FailureResponse& response_;
};
}
