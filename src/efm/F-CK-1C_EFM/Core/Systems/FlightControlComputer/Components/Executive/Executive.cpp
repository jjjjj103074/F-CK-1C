#include "Executive.h"

#include "../AmuxProcessor/AmuxProcessor.h"
#include "../ControlLaws/ControlLaws.h"
#include "../FailureManager/FailureManager.h"
#include "../SelectorMonitor/SelectorMonitor.h"
#include "../StartupAndRestart/StartupAndRestart.h"
#include "../SystemMonitor/SystemMonitor.h"

#include <limits>
#include <stdexcept>

namespace Core::Systems::Flcc
{
    Executive::Executive(ExecutiveTables &tables,
                         StartupAndRestart &startup_and_restart,
                         AmuxProcessor &amux_processor,
                         SystemMonitor &system_monitor,
                         SelectorMonitor &selector_monitor,
                         FailureManager &failure_manager,
                         ControlLaws &control_laws)
        // 任務排程設定
        : Executive(tables, {// 尚未定義實機任務頻率與完整資料依賴；目前各列皆為 64 Hz 骨架入口。
                             {"startup_and_restart.step", TaskFrequency(64),
                              startup_and_restart, &StartupAndRestart::step},
                             {"amux_processor.step", TaskFrequency(64),
                              amux_processor, &AmuxProcessor::step},
                             {"system_monitor.step", TaskFrequency(64),
                              system_monitor, &SystemMonitor::step},
                             {"selector_monitor.step", TaskFrequency(64),
                              selector_monitor, &SelectorMonitor::step},
                             {"failure_manager.step", TaskFrequency(64),
                              failure_manager, &FailureManager::step},
                             {"control_laws.step", TaskFrequency(64),
                              control_laws, &ControlLaws::step}})
    {
    }

    Executive::Executive(ExecutiveTables &tables,
                         std::initializer_list<TaskDefinition> definitions)
        : tables_(tables)
    {
        tables_.initialize(definitions);
    }

    ExecutiveTask *Executive::find_next_due(TaskScheduledTime target_time)
    {
        ExecutiveTask *due = nullptr;
        for (ExecutiveTask &task : tables_.tasks_)
        {
            if (task.state.next_due <= target_time &&
                (due == nullptr || task.state.next_due < due->state.next_due))
            {
                due = &task;
            }
        }
        return due;
    }

    void Executive::step(TaskScheduledTime target_time)
    {
        // 避免時間倒退
        if (target_time < tables_.scheduler_state_.advanced_through)
        {
            throw std::invalid_argument("FLCC executive time must not move backwards.");
        }

        // 找到下一個已經到期的任務
        while (ExecutiveTask *task = find_next_due(target_time)) //
        {
            const TaskDefinition &definition = task->definition;
            TaskRuntimeState &state = task->state;

            // 防止溢位
            if (state.completed_ticks >= (std::numeric_limits<std::uint64_t>::max)() - 1)
            {
                throw std::overflow_error("FLCC task invocation count exceeded its range.");
            }

            const TaskScheduledTime following_due_time =
                definition.frequency.tick_time(state.completed_ticks + 2);       // 計算下一次執行時間
            definition.step_(state.next_due, definition.frequency.period_s()); // 執行任務入口
            ++state.completed_ticks;                                             // 更新已完成呼叫次數
            state.next_due = following_due_time;                                           // 更新下一次到期時間
        }

        // 更新上次時間
        tables_.scheduler_state_.advanced_through = target_time;
    }
}
