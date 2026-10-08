#pragma once

#include "../System.h"
#include "Components/AmuxProcessor/AmuxProcessor.h"
#include "Components/ControlLaws/ControlLaws.h"
#include "Components/Executive/Executive.h"
#include "Components/FailureManager/FailureManager.h"
#include "Components/SelectorMonitor/SelectorMonitor.h"
#include "Components/StartupAndRestart/StartupAndRestart.h"
#include "Components/SystemMonitor/SystemMonitor.h"
#include "Database/AmuxData.h"
#include "Database/ControlLawData.h"
#include "Database/ExecutiveTables.h"
#include "Database/InputOutputData.h"
#include "Database/SystemStatusTables.h"

namespace Core::Systems
{
    /// @brief 一台飛控電腦的組裝與 Pipeline 邊界；直接持有七個元件及五個資料庫。
    /// 建構時由 StartupAndRestart 啟用 Executive；五個週期元件提供 step 入口。
    /// 目前提供三軸正規化直接映射；尚未實作回授、配平、模式、監控、故障與 AMUX。
    /// 尚未實作冷啟動檢查、斷電恢復與故障重啟。
    /// 完整系統狀態的計算尚未實作；對外狀態維持不可用。
    class FlightControlComputer final : public System
    {
    public:
        /// @brief 建立資料庫、元件與固定任務表，完成初始輸出後啟用排程。
        /// @param initial_throttle_levers 控制需求計算完成前保留的左右油門初值，範圍為 0 到 1。
        explicit FlightControlComputer(const ThrottleLeverSignal &initial_throttle_levers);

        // 資料庫參照與任務的成員函式綁定使用此實例的固定地址。
        FlightControlComputer(const FlightControlComputer &) = delete;
        FlightControlComputer &operator=(const FlightControlComputer &) = delete;
        FlightControlComputer(FlightControlComputer &&) = delete;
        FlightControlComputer &operator=(FlightControlComputer &&) = delete;

        /// @brief 設定供電狀態；只控制 step 是否執行，不執行啟動或重啟程序。
        /// @param powered true 表示有電，false 表示沒電。
        void set_powered(bool powered) noexcept;

        /// @brief 取得目前供電狀態；實際電源系統的連接尚未實作。
        /// @return true 表示有電，false 表示沒電。
        bool powered() const noexcept;

        /// @brief 宣告外層頻率、五個輸入與四個輸出。
        /// @param setup Pipeline 提供的系統宣告介面。
        void setup(SystemSetup &setup) override;

        /// @brief 有電時接收輸入、推進 Executive 並發布輸出；沒電時直接返回。
        /// @param context 外層的預定模擬時間；內部任務週期由各任務頻率決定。
        /// @param aircraft Pipeline 提供的唯讀飛機資料快照。
        /// @param result 四個具型別輸出的發布介面。
        void step(const SystemStepContext &context,
                  const AircraftDataView &aircraft, SystemResult &result) override;

    private:
        bool powered_ = true; ///< 供電狀態；預設有電，由呼叫端設定。

        /// @brief 將已宣告的 Pipeline 輸入複製至固定的輸入分區。
        /// @param aircraft 本次外層 step 的飛機資料快照。
        void read_inputs(const AircraftDataView &aircraft);

        /// @brief 發布正規化舵面需求、油門初值與尚未完成的系統狀態。
        /// @param result Pipeline 提供的輸出介面。
        void publish_outputs(SystemResult &result) const;

        // C++ 成員依宣告順序建構、反向銷毀；資料庫的生命週期涵蓋所有元件。
        Flcc::ExecutiveTables executive_tables_;        ///< 任務設定與執行狀態；設定固定，狀態由 Executive 更新。
        Flcc::ControlLawData control_law_data_;         ///< 唯讀控制律設定與目前執行狀態。
        Flcc::InputOutputData input_output_data_;       ///< 與 Pipeline 交換的輸入及輸出。
        Flcc::SystemStatusTables system_status_tables_; ///< 元件共用的系統狀態。
        Flcc::AmuxData amux_data_;                      ///< AMUX 資料的分區骨架。

        Flcc::AmuxProcessor amux_processor_;          ///< AMUX 處理元件。
        Flcc::SystemMonitor system_monitor_;          ///< 系統監控元件。
        Flcc::SelectorMonitor selector_monitor_;      ///< 資料選擇監控元件。
        Flcc::FailureManager failure_manager_;        ///< 故障管理元件。
        Flcc::ControlLaws control_laws_;              ///< 控制律元件。
        Flcc::Executive executive_;                   ///< 在受排程元件之後建立的內部排程器。
        Flcc::StartupAndRestart startup_and_restart_; ///< 在 Executive 之後建立的啟動元件。
    };
}
