#pragma once

#include <chrono>

namespace Core::Systems::Mc
{
struct InputOutputData;
struct HealthStatus;

/// @brief MC 健康監控骨架；唯讀輸入輸出分區，只能寫入健康分區。
/// 資料由 MC 持有；本元件不自行註冊任務或管理排程頻率。
class SystemMonitor final
{
public:
	SystemMonitor(const InputOutputData& input_output, HealthStatus& health);
	/// @brief 接收一次排程呼叫；尚未執行檢查或寫入監控結果。
	void step(std::chrono::nanoseconds scheduled_time, double dt_s);

private:
	const InputOutputData& input_output_;
	HealthStatus& health_;
};
}
