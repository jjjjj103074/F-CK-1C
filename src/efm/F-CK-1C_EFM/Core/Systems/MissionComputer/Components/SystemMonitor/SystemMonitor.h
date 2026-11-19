#pragma once

#include <chrono>

namespace Core::Systems::Mc
{
struct InputOutputData;
struct HealthStatus;

/// @brief 綁定輸入輸出與健康分區，提供監控的排程入口。
/// 資料由 MC 持有。
class SystemMonitor final
{
public:
	SystemMonitor(const InputOutputData& input_output, HealthStatus& health);
	/// @brief 接收排程時間與週期。
	void step(std::chrono::nanoseconds scheduled_time, double dt_s);

private:
	const InputOutputData& input_output_;
	HealthStatus& health_;
};
}
