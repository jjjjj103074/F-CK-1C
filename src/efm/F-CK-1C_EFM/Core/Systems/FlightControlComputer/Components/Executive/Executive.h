#pragma once

#include "../../Database/ExecutiveTables.h"

#include <chrono>

namespace Core::Systems::Flcc
{
class StartupAndRestart;
class AmuxProcessor;
class SystemMonitor;
class SelectorMonitor;
class FailureManager;
class ControlLaws;

/// @brief 一台 FLCC 的單執行緒排程器；集中定義任務表並按絕對模擬時間呼叫 step。
/// 設定與執行狀態均保存於 ExecutiveTables；不擁有受排程元件。
class Executive final
{
public:
    /// @brief 建立此 FLCC 的固定任務表；頻率與列順序集中於 Executive.cpp。
    /// @param tables 由 FLCC 持有、尚未初始化的排程分區。
    /// @param startup_and_restart 啟動與重新啟動元件。
    /// @param amux_processor AMUX 處理元件。
    /// @param system_monitor 系統監控元件。
    /// @param selector_monitor 資料選擇監控元件。
    /// @param failure_manager 故障管理元件。
    /// @param control_laws 控制律元件。
    /// 所有參照目標必須持續存在且地址不變，直到此 Executive 不再使用。
    Executive(ExecutiveTables &tables,
              StartupAndRestart &startup_and_restart,
              AmuxProcessor &amux_processor,
              SystemMonitor &system_monitor,
              SelectorMonitor &selector_monitor,
              FailureManager &failure_manager,
              ControlLaws &control_laws);

    /// @brief 由明確指定的任務列建立排程；可用於不同設定與排程行為驗證。
    /// @param tables 尚未初始化的排程分區，存活時間須涵蓋此 Executive。
    /// @param definitions 已綁定入口的任務設定；傳入的列順序直接決定同時到期順序。
    /// @throws std::logic_error 分區已初始化。
    /// @throws std::invalid_argument 任務設定無效或名稱重複。
    Executive(ExecutiveTables &tables, std::initializer_list<TaskDefinition> definitions);
    Executive(const Executive &) = delete;
    Executive &operator=(const Executive &) = delete;
    Executive(Executive &&) = delete;
    Executive &operator=(Executive &&) = delete;

    /// @brief 執行所有不晚於 target_time 的任務，包含外層延遲呼叫造成的欠期工作。
    /// @param target_time 單調不減的絕對模擬時間，單位為奈秒。
    /// 同時到期依表格列順序執行；重複目標時間不會重複執行任務。
    /// @throws std::invalid_argument 時間為負或向後移動。
    /// @throws std::overflow_error 任務時鐘超出可表示範圍。
    /// 任務例外直接向上傳遞；尚未實作故障恢復或資料交易回復。
    void step(std::chrono::nanoseconds target_time);

private:
    /// @brief 找出最早到期且不晚於目標時間的任務；同時到期保留列順序。
    /// @return 可更新狀態的任務列；沒有到期任務時回傳 nullptr。
    ExecutiveTask *find_next_due(std::chrono::nanoseconds target_time);

    ExecutiveTables &tables_; ///< FLCC 持有的任務設定及執行進度。
};
}
