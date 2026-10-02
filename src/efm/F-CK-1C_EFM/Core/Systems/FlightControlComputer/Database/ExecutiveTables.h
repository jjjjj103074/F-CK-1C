#pragma once

#include "../Components/Executive/TaskFrequency.h"

#include <functional>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Core::Systems::Flcc
{
class Executive;

/// @brief 一個任務的固定設定；存入 ExecutiveTask 後由 const 保護。
struct TaskDefinition
{
    std::string id; ///< 全 FLCC 唯一的任務名稱。
    TaskFrequency frequency; ///< 以正整數分數表示的 Hz。

    /// @brief 將任務入口綁定至既有元件；不擁有或複製元件。
    /// @tparam Component 具有指定 step 成員函式的元件型別。
    /// @param id 全 FLCC 唯一的任務名稱。
    /// @param frequency 此任務自身的精確頻率。
    /// @param component 在排程表使用期間持續存在且地址不變的元件。
    /// @param method 接收預定模擬時間與固定週期的成員函式。
    /// @throws std::invalid_argument 名稱為空或成員函式指標為空。
    template <typename Component>
    TaskDefinition(std::string id, TaskFrequency frequency, Component &component,
                   void (Component::*method)(TaskScheduledTime, double))
        : id(std::move(id)), frequency(frequency),
          step_([target = &component, method](TaskScheduledTime scheduled_time, double dt_s)
          {
              (target->*method)(scheduled_time, dt_s);
          })
    {
        if (this->id.empty() || method == nullptr)
        {
            throw std::invalid_argument("FLCC task definition is incomplete.");
        }
    }

private:
    std::function<void(TaskScheduledTime, double)> step_; ///< 已綁定的任務入口。
    friend class ExecutiveTables;
    friend class Executive;
};

/// @brief 一個任務的可變執行狀態；僅由 Executive 更新。
struct TaskRuntimeState
{
    std::uint64_t completed_ticks = 0; ///< 已成功返回的呼叫次數，為排程進度的依據。
    TaskScheduledTime next_due = {}; ///< 由 completed_ticks + 1 推導的下次到期時間快取。
};

/// @brief 同一個任務的固定設定與目前狀態；表格列順序即同時到期的執行順序。
struct ExecutiveTask
{
    const TaskDefinition definition; ///< 建立後不可變的任務設定。
    TaskRuntimeState state; ///< 與此設定放在同一列的執行狀態。

    /// @brief 建立設定及首次到期時間；零時刻不執行任務。
    explicit ExecutiveTask(const TaskDefinition &definition);

    /// @brief 查詢距離下次預定執行的模擬時間；負值表示已到期但尚未處理。
    /// @param reference_time 查詢所依據的非負模擬時間，非實際 CPU 時間。
    /// @throws std::invalid_argument 查詢時間為負。
    TaskScheduledTime time_until_next(TaskScheduledTime reference_time) const;
};

/// @brief 整份排程表的執行狀態。
struct ExecutiveSchedulerState
{
    TaskScheduledTime advanced_through = {}; ///< 最近一次成功完成的外層目標模擬時間。
};

/// @brief 一台 FLCC 的排程資料分區；每列同時保存任務設定與執行狀態。
/// Executive 一次建立任務表並更新狀態；其他使用者只能取得唯讀參照。
/// 任務綁定不擁有元件；元件必須比使用此表的 Executive 更晚銷毀。
class ExecutiveTables final
{
public:
    ExecutiveTables() = default;
    ExecutiveTables(const ExecutiveTables &) = delete;
    ExecutiveTables &operator=(const ExecutiveTables &) = delete;
    ExecutiveTables(ExecutiveTables &&) = delete;
    ExecutiveTables &operator=(ExecutiveTables &&) = delete;

    /// @brief 取得依工程師填表順序排列的唯讀任務列；設定與狀態可一次查看。
    const std::vector<ExecutiveTask> &tasks() const noexcept;

    /// @brief 取得整份排程表的唯讀狀態。
    const ExecutiveSchedulerState &scheduler_state() const noexcept;

private:
    /// @brief 由 Executive 一次建立所有任務列；不提供新增、排序或重新設定介面。
    /// @throws std::logic_error 此分區已被 Executive 初始化。
    /// @throws std::invalid_argument 名稱為空、入口無效或任務名稱重複。
    void initialize(std::initializer_list<TaskDefinition> definitions);

    std::vector<ExecutiveTask> tasks_; ///< 固定列順序；每列的 definition 為 const。
    ExecutiveSchedulerState scheduler_state_; ///< 排程器整體進度的唯一儲存位置。
    bool initialized_ = false; ///< 防止同一分區被再次初始化，包含空任務表。

    friend class Executive;
};
}
