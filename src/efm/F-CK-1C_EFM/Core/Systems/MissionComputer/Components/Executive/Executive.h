#pragma once

#include "../../Database/ExecutiveTables.h"

#include <chrono>

namespace Core::Systems::Mc
{
class SystemMonitor;
class FailureManager;

/// @brief 一台 MC 的單執行緒排程器；任務定義、時鐘及補跑規則對齊 DFLCC。
/// 不擁有受排程元件；所有狀態保存在 MC 的 ExecutiveTables。
class Executive final
{
public:
	/// @brief 以 SystemMonitor、FailureManager 的順序建立固定骨架任務表。
	/// 所有參照目標必須持續存在且地址不變；暫定頻率集中於 Executive.cpp。
	Executive(ExecutiveTables& tables, SystemMonitor& system_monitor, FailureManager& failure_manager);
	/// @brief 由明確指定的任務入口與頻率建立排程；同時到期依傳入列順序執行。
	/// @throws std::logic_error 分區已初始化。
	/// @throws std::invalid_argument 任務設定無效或名稱重複。
	Executive(ExecutiveTables& tables, std::initializer_list<TaskDefinition> definitions);
	Executive(const Executive&) = delete;
	Executive& operator=(const Executive&) = delete;
	Executive(Executive&&) = delete;
	Executive& operator=(Executive&&) = delete;

	/// @brief 啟用排程；以本架飛機建立時為零時刻，首個任務在完整週期後執行。
	/// @throws std::logic_error 排程已啟動；尚未實作重啟或時間基準重設。
	void start();
	/// @brief 推進至單調不減的絕對模擬時間，補跑全部到期任務。
	/// @throws std::logic_error 尚未啟動。
	/// @throws std::invalid_argument 時間為負或倒退。
	/// @throws std::overflow_error 任務時鐘超出範圍。
	/// 例外直接向上傳遞；沒有交易回復或故障恢復。
	void step(std::chrono::nanoseconds target_time);

private:
	ExecutiveTask* find_next_due(std::chrono::nanoseconds target_time);
	ExecutiveTables& tables_;
};
}
