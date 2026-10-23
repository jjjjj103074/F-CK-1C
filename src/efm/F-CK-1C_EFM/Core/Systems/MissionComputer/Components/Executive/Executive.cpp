#include "Executive.h"

#include "../FailureManager/FailureManager.h"
#include "../SystemMonitor/SystemMonitor.h"
#include "../../../SystemUpdateRates.h"

#include <limits>
#include <stdexcept>

namespace Core::Systems::Mc
{
Executive::Executive(ExecutiveTables& tables, SystemMonitor& system_monitor, FailureManager& failure_manager)
    : Executive(tables,
                { // 專案骨架頻率，並非實機 MC 規格；監控先於故障處置。
                  { "system_monitor.step",
                    TaskFrequency(kProjectDefinedFallbackUpdateRateHz),
                    system_monitor,
                    &SystemMonitor::step },
                  { "failure_manager.step",
                    TaskFrequency(kProjectDefinedFallbackUpdateRateHz),
                    failure_manager,
                    &FailureManager::step } })
{
}

Executive::Executive(ExecutiveTables& tables, std::initializer_list<TaskDefinition> definitions)
    : tables_(tables)
{
	tables_.initialize(definitions);
}

void Executive::start()
{
	if (tables_.scheduler_state_.started)
	{
		throw std::logic_error("MC executive is already started.");
	}
	tables_.scheduler_state_.started = true;
}

ExecutiveTask* Executive::find_next_due(std::chrono::nanoseconds target_time)
{
	ExecutiveTask* due = nullptr;
	for (ExecutiveTask& task : tables_.tasks_)
	{
		if (task.state.next_due <= target_time && (due == nullptr || task.state.next_due < due->state.next_due))
		{
			due = &task;
		}
	}
	return due;
}

void Executive::step(std::chrono::nanoseconds target_time)
{
	if (!tables_.scheduler_state_.started)
	{
		throw std::logic_error("MC executive has not been started.");
	}

	// 避免時間倒退
	if (target_time < tables_.scheduler_state_.advanced_through)
	{
		throw std::invalid_argument("MC executive time must not move backwards.");
	}

	// 找到下一個已經到期的任務
	while (ExecutiveTask* task = find_next_due(target_time))
	{
		const TaskDefinition& definition = task->definition;
		TaskRuntimeState& state = task->state;

		// 防止溢位
		if (state.completed_ticks >= (std::numeric_limits<std::uint64_t>::max)() - 1)
		{
			throw std::overflow_error("MC task invocation count exceeded its range.");
		}

		const std::chrono::nanoseconds following_due_time =
		    definition.frequency.tick_time(state.completed_ticks + 2); // 計算下一次執行時間
		definition.step_(state.next_due, definition.frequency.period_s()); // 執行任務入口
		++state.completed_ticks; // 更新已完成呼叫次數
		state.next_due = following_due_time; // 更新下一次到期時間
	}

	// 更新上次時間
	tables_.scheduler_state_.advanced_through = target_time;
}
}
