#pragma once

namespace Core::Systems::Mc
{
class Executive;
struct LifecycleStatus;

/// @brief MC 啟動程序；目前與 DFLCC 一樣只啟用 Executive，不參與週期排程。
/// 尚未實作冷啟動檢查、重設、斷電恢復或故障重啟。
class StartupAndRestart final
{
public:
	StartupAndRestart(Executive& executive, LifecycleStatus& lifecycle);
	/// @brief 啟用 Executive，不執行任務；生命週期分區目前仍為空骨架。
	/// @throws std::logic_error Executive 已啟動。
	void start();

private:
	Executive& executive_;
	LifecycleStatus& lifecycle_;
};
}
