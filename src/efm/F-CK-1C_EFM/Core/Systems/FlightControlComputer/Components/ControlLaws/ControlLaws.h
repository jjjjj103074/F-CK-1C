#pragma once

#include <chrono>

namespace Core::Systems::Flcc
{
    struct InputOutputData;
    struct ControlLawData;
    struct SystemStatusTables;

    /// @brief 執行 FLCC 控制律的元件。
    /// 目前實作：三軸曲線與增益直接映射至正規化舵面位置需求。
    /// 尚未實作：回授控制、配平、模式、濾波與積分。
    /// 建構時綁定三個資料庫參照，任務由 Executive 排程表安排。
    class ControlLaws final
    {
    public:
        /// @brief 綁定此元件固定使用的資料庫，其生命週期涵蓋元件生命週期。
        /// @param input_output 供讀取輸入與寫入控制需求的分區。
        /// @param control_law 提供唯讀設定與可更新執行狀態的分區。
        /// @param system_status 唯讀的系統狀態分區。
        ControlLaws(
            InputOutputData &input_output,
            ControlLawData &control_law,
            const SystemStatusTables &system_status);

        /// @brief 依固定設定映射三軸輸入，寫入完整需求並更新限幅狀態。
        /// @param scheduled_time 此任務本次預定的絕對模擬時間。
        /// @param dt_s 此任務自身的固定週期，單位為秒。
        /// 直接映射依本次輸入與設定計算，時間參數符合 Executive 的任務介面。
        void step(std::chrono::nanoseconds scheduled_time, double dt_s);

    private:
        InputOutputData &input_output_;           ///< 供讀取輸入與寫入控制需求的分區。
        ControlLawData &control_law_;             ///< 唯讀設定與本次運算的限幅狀態。
        const SystemStatusTables &system_status_; ///< 固定綁定；尚未實作依系統狀態選擇模式。
    };
}
