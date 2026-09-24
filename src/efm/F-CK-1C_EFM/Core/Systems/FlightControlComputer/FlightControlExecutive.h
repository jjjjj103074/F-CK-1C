#pragma once

#include "CommandSystem/FlightControlCommandSystem.h"
#include "CommandSystem/FlightControlCommandBinding.h"
#include "Configuration/FlightControlComputerConfig.h"
#include "Diagnostics/FlightControlDiagnostics.h"
#include "Output/FlightControlOutputSystem.h"
#include "Util/FlightStateComputation.h"
#include "Input/InputSignalManagement.h"
#include "Util/ThrottleCommandComposition.h"
#include "ControlLaws/ControlLaws.h"

#include <vector>

namespace Core::Systems
{
    /// @brief 一次 FLCC 計算的完整輸入；數值已轉成 Core 約定的單位。
    /// 不含 Pipeline 資料鍵或 DCS 原始軸值。
    struct FlightControlComputerStepInput
    {
        RawFlightControlInput flight_control; // 觀測、駕駛控制、起落架、致動器回授與本週期 dt。
        ThrottleLeverSignal throttle_levers;  // 左右油門桿位置，供直通或開發用自動油門使用。
    };

    // 同一個 FLCC 週期完成後的結果；前兩項是控制命令，後兩項是狀態快照。
    struct FlightControlComputerResult
    {
        FlightControlActuatorCommand actuator_command;           // 控制面電子需求，單位為弧度。
        EngineThrottleCommand engine_throttle_command;           // 左右引擎的正規化油門命令。
        FlightControlComputerSnapshot diagnostics;               // FLCC 控制狀態與診斷快照。
        AutomaticFlightControlSnapshot automatic_flight_control; // 自動飛行模式與參考值快照。
    };

    // 收集已完成的本週期中間結果，供診斷投影使用；不回寫控制律。
    struct FlightControlExecutiveDiagnosticsInput
    {
        const FlightControlCommandSystemResult &command;  // 飛行員／自動飛行指令選擇與協調結果。
        const ::Systems::FlightControlLawsResult &laws;  // 三軸控制律計算結果。
        const ::Systems::ActiveFlightControlConfiguration &configuration;  // 本週期選定的 CAT、增益與包線設定。
        const ::Systems::ManagedFlightControlSignals &signals;  // 濾波、塑形後的觀測與飛行員訊號。
        const FlightControlOutputStatus &output_status;  // 電子輸出限制與致動器追蹤狀態。
    };

    // 已完成控制計算後，交給自動飛行監測器評估的資料。
    struct AutopilotMonitorInput
    {
        const ::Systems::ComputedFlightState &flight;  // 本週期計算出的飛行狀態。
        const FlightControlCommandSystemResult &command;  // 本週期選擇的控制指令及自動飛行參考值。
        const ::Systems::FlightControlLawsResult &laws;  // 控制律是否進入硬性保護的來源。
        bool control_path_saturated = false;  // 電子命令或實際致動器有任一飽和時為 true。
    };

    // FLCC 內部的執行鏈：擁有計算模組，決定呼叫順序與子頻率。
    // 它不是另一個飛機 System，也不直接發布資料到外部 SystemPipeline。
    class FlightControlExecutive
    {
    public:
        /// @brief 建立一架飛機的 FLCC 執行鏈與初始輸出。
        /// @param config 已完成驗證的完整 FLCC 設定；只在建構時分配給各模組。
        /// @param start_mode 飛行開始方式，用於自動飛行的初始接地判斷。
        /// @param initial_throttle_levers 首次更新前的左右油門初值，範圍為 0 到 1。
        FlightControlExecutive(
            const FlightControlComputerConfig &config,
            StartMode start_mode,
            const ThrottleLeverSignal &initial_throttle_levers);

        /// @brief 讀取建構時彙整的指令 ID 與處理函式，供 Computer 登記。
        /// @return 本實例持有的綁定；物件存續期間內容不再改變。
        const std::vector<FlightControlCommandBinding> &command_bindings() const;

        /// @brief 依固定順序執行本週期的輸入、模式、控制律與輸出計算。
        /// @param input 一個 FLCC 週期的完整 Core 輸入。
        /// @return 本物件持有的完整結果參照；下一次 update 會覆寫內容。
        const FlightControlComputerResult &update(
            const FlightControlComputerStepInput &input);

        /// @brief 讀取最近一次完整結果，不觸發新運算。
        /// @return 最近一次結果參照；首次週期前為建構時的初值。
        const FlightControlComputerResult &result() const;

    private:
        /// @brief 更新塑形與增益排程的低頻週期計數。
        /// @param shaping_tick 本週期是否執行飛行員輸入塑形。
        /// @param gain_tick 本週期是否執行慢頻增益排程。
        void update_subrate_counters(bool shaping_tick, bool gain_tick);

        /// @brief 合成左右引擎油門命令並存入本週期結果。
        /// @param levers 飛行員左右油門桿的正規化位置。
        /// @param automatic 開發用自動油門的參考值與可用狀態。
        void update_engine_throttle(
            const ThrottleLeverSignal &levers,
            const AutomaticFlightGuidanceReference &automatic);

        /// @brief 將已完成的控制結果轉為診斷快照。
        /// @param input 本週期的指令、控制律、選定設定與電子輸出狀態。
        void update_diagnostics(
            const FlightControlExecutiveDiagnosticsInput &input);

        /// @brief 組成自動飛行監測器需要的觀測資料。
        /// @param input 飛行狀態、指令、控制律與飽和資訊。
        /// @return 僅供自動飛行監測的具型別觀測資料。
        FlightControlCommandMonitorInput make_monitor_observation(
            const AutopilotMonitorInput &input) const;

        /// @brief 依優先順序將硬性保護狀態轉成自動飛行約束原因。
        /// @param status 控制律回報的限制狀態。
        /// @return 最優先的約束原因；沒有硬性保護時回傳無原因。
        ConstraintReason hard_protection_reason(
            const ::Systems::FlightControlLawsStatus &status) const;

        const bool developer_direct_control_law_;  // 是否使用開發用直接控制律輸出。
        InputSignalManagement input_signals_;  // 輸入驗證、觀測濾波及輸入塑形的狀態擁有者。
        ::Systems::ModeAndGainScheduling mode_and_gain_;  // CAT 模式與飛行條件增益排程的狀態擁有者。
        FlightControlCommandSystem command_system_;  // 飛行命令、自動飛行及監測的狀態擁有者。
        ::Systems::FlightControlLaws flight_control_laws_;  // 三軸控制律與電子控制面需求的狀態擁有者。
        FlightControlOutputSystem output_system_;  // 電子輸出選擇、轉換與限制的狀態擁有者。
        FlightControlDiagnostics diagnostics_;  // 將本週期結果投影成 FLCC 診斷快照。
        ::Systems::PilotInputShapingConfig held_pilot_shaping_;  // 最近一次慢頻輸入塑形設定。
        FlightControlComputerResult result_;  // 最近一次完整輸出；update 回傳的參照指向此值。
        std::vector<FlightControlCommandBinding> command_bindings_;  // 建構時彙整的指令綁定。
        std::uint64_t tick_ = 0;  // 從零開始的 64 Hz 週期序號。
        std::uint64_t pilot_shaping_update_count_ = 0;  // 已執行的飛行員輸入塑形次數。
        std::uint64_t pilot_shaping_last_update_tick_ = 0;  // 最近執行飛行員輸入塑形的週期。
        std::uint64_t gain_schedule_update_count_ = 0;  // 已執行的慢頻增益排程次數。
        std::uint64_t gain_schedule_last_update_tick_ = 0;  // 最近執行慢頻增益排程的週期。
    };
}
