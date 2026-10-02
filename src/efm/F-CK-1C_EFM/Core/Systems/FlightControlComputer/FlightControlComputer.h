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
    /// Executive 管理內部任務時間；其餘元件提供 step 入口並持有固定資料庫參照。
    /// 尚未實作：控制律、監控、啟動、故障與 AMUX 運算；對外狀態維持不可用。
    class FlightControlComputer final : public System
    {
    public:
        /// @brief 建立資料庫、元件與封存後的任務表。
        /// @param initial_throttle_levers 控制需求計算完成前保留的左右油門初值，範圍為 0 到 1。
        explicit FlightControlComputer(const ThrottleLeverSignal &initial_throttle_levers);

        // 資料庫參照與成員函式綁定均依賴實例地址，禁止複製或搬移整台 FLCC。
        FlightControlComputer(const FlightControlComputer &) = delete;
        FlightControlComputer &operator=(const FlightControlComputer &) = delete;
        FlightControlComputer(FlightControlComputer &&) = delete;
        FlightControlComputer &operator=(FlightControlComputer &&) = delete;

        /// @brief 宣告外層頻率、五個輸入與四個輸出；本階段沒有指令處理器。
        /// @param setup Pipeline 提供的系統宣告介面。
        void setup(SystemSetup &setup) override;

        /// @brief 接收輸入、推進 Executive，並發布目前資料庫中的輸出。
        /// @param context 外層的預定模擬時間；內部任務不使用外層 dt 決定自身週期。
        /// @param aircraft Pipeline 提供的唯讀飛機資料快照。
        /// @param result 四個具型別輸出的發布介面。
        void step(const SystemStepContext &context,
                  const AircraftDataView &aircraft, SystemResult &result) override;

    private:
        /// @brief 將已宣告的 Pipeline 輸入複製至固定的輸入分區。
        /// @param aircraft 本次外層 step 的飛機資料快照。
        void read_inputs(const AircraftDataView &aircraft);

        /// @brief 發布控制需求及狀態；骨架狀態仍為不可用。
        /// @param result Pipeline 提供的輸出介面。
        void publish_outputs(SystemResult &result) const;

        // C++ 成員依宣告順序建構、反向銷毀；資料庫必須先於元件建立。
        Flcc::ExecutiveTables executive_tables_;        ///< 任務設定與執行狀態；設定固定，狀態由 Executive 更新。
        Flcc::ControlLawData control_law_data_;         ///< 共用控制律資料的分區骨架。
        Flcc::InputOutputData input_output_data_;       ///< 與 Pipeline 交換的輸入及輸出。
        Flcc::SystemStatusTables system_status_tables_; ///< 元件共用的系統狀態。
        Flcc::AmuxData amux_data_;                      ///< AMUX 資料的分區骨架。

        Flcc::StartupAndRestart startup_and_restart_; ///< 啟動與重新啟動元件。
        Flcc::AmuxProcessor amux_processor_;          ///< AMUX 處理元件。
        Flcc::SystemMonitor system_monitor_;          ///< 系統監控元件。
        Flcc::SelectorMonitor selector_monitor_;      ///< 資料選擇監控元件。
        Flcc::FailureManager failure_manager_;        ///< 故障管理元件。
        Flcc::ControlLaws control_laws_;              ///< 控制律元件。
        Flcc::Executive executive_;                   ///< 最後建立、最先銷毀的內部排程器。
    };
}
