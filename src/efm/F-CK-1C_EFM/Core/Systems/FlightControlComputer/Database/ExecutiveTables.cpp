#include "ExecutiveTables.h"

#include <algorithm>

namespace Core::Systems::Flcc
{
    ExecutiveTask::ExecutiveTask(const TaskDefinition &definition)
        : definition(definition), state{0, definition.frequency.tick_time(1)}
    {
    }

    TaskScheduledTime ExecutiveTask::time_until_next(TaskScheduledTime reference_time) const
    {
        // 檢查參考時間不為負值
        if (reference_time < TaskScheduledTime::zero())
        {
            throw std::invalid_argument("FLCC task query time must not be negative.");
        }
        return state.next_due - reference_time;
    }

    void ExecutiveTables::initialize(std::initializer_list<TaskDefinition> definitions)
    {
        // 檢查是否已初始化
        if (initialized_)
        {
            throw std::logic_error("FLCC executive tables are already initialized.");
        }

        std::vector<ExecutiveTask> initial;
        initial.reserve(definitions.size());

        // 驗證每個任務定義的完整性與唯一性
        for (const TaskDefinition &definition : definitions)
        {
            // 檢查任務定義是否完整
            if (definition.id.empty() || !definition.step_)
            {
                throw std::invalid_argument("FLCC task definition is incomplete.");
            }

            // 檢查任務名稱是否重複
            const bool duplicate = std::any_of(initial.begin(), initial.end(),
                                               [&definition](const ExecutiveTask &task)
                                               {
                                                   return task.definition.id == definition.id;
                                               });
            if (duplicate)
            {
                throw std::invalid_argument("Duplicate FLCC task ID: " + definition.id);
            }

            // 將任務定義加入初始列表
            initial.emplace_back(definition);
        }

        // 所有列都完成驗證後才採用新表；設定在各列建構時完成，之後不再修改。
        tasks_.swap(initial);
        initialized_ = true;
    }

    const std::vector<ExecutiveTask> &ExecutiveTables::tasks() const noexcept
    {
        return tasks_;
    }

    const ExecutiveSchedulerState &ExecutiveTables::scheduler_state() const noexcept
    {
        return scheduler_state_;
    }
}
