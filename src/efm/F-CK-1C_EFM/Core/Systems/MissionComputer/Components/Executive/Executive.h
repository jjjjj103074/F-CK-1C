#pragma once

#include "../../Database/ExecutiveTables.h"

#include <chrono>

namespace Core::Systems::Mc
{
class SystemMonitor;
class FailureManager;

/// @brief MC 單執行緒排程器，依任務表推進時鐘並補跑到期工作。
/// 排程狀態保存在 MC 持有的 ExecutiveTables。
class Executive final
{
public:
	/// @brief 以 SystemMonitor、FailureManager 的順序建立固定骨架任務表。
	/// 元件以固定地址供排程器呼叫，頻率集中於 Executive.cpp。
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
	/// @throws std::logic_error 排程已啟動。
	void start();
	/// @brief 推進至單調不減的絕對模擬時間，補跑全部到期任務。
	/// @throws std::logic_error 尚未啟動。
	/// @throws std::invalid_argument 時間為負或倒退。
	/// @throws std::overflow_error 任務時鐘超出範圍。
	/// 任務例外向呼叫端傳遞。
	void step(std::chrono::nanoseconds target_time);

private:
	ExecutiveTask* find_next_due(std::chrono::nanoseconds target_time);
	ExecutiveTables& tables_;
};
}
