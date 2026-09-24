#pragma once

#include "Configuration/FlightControlComputerConfig.h"
#include "Diagnostics/FlightControlComputerDebugTelemetry.h"
#include "FlightControlExecutive.h"
#include "../System.h"

namespace Core::Systems
{
/// @brief FLCC 與 SystemPipeline 之間的轉接器，負責讀取輸入並發布輸出。
/// 運算順序與跨週期狀態由持有的 FlightControlExecutive 管理。
class FlightControlComputer final : public System
{
public:
	/// @brief 使用模組內的完整 F-CK-1C 設定建立一架飛機的 FLCC。
	/// @param start_mode 飛行開始方式，決定自動飛行的初始接地狀態。
	/// @param initial_throttle_levers 首次更新前的左右油門桿位置，範圍為 0 到 1。
	FlightControlComputer(
		StartMode start_mode,
		const ThrottleLeverSignal& initial_throttle_levers);
	/// @brief 使用指定的完整設定建立 FLCC，供測試注入設定差異。
	/// @param config 已合併、驗證及封存的完整設定；Executive 只向子模組分配所需部分。
	/// @param start_mode 飛行開始方式，決定自動飛行的初始接地狀態。
	/// @param initial_throttle_levers 首次更新前的左右油門桿位置，範圍為 0 到 1。
	FlightControlComputer(
		const FlightControlComputerConfig& config,
		StartMode start_mode,
		const ThrottleLeverSignal& initial_throttle_levers);
	/// @brief 登記 64 Hz 更新率、五種輸入、四種初始輸出及指令處理器。
	/// @param setup Pipeline 提供的系統宣告介面；建構期間呼叫一次。
	void setup(SystemSetup& setup) override;
	/// @brief 執行一個排程週期，將 Executive 結果發布到 Pipeline。
	/// @param context 本週期的模擬時間與時間間隔，後者以秒為單位。
	/// @param aircraft 本週期可讀取的五種具型別飛機資料快照。
	/// @param result 本週期四種具型別輸出的發布介面。
	void step(
		const SystemStepContext& context,
		const AircraftDataView& aircraft,
		SystemResult& result) override;

private:
	/// @brief 將排程時間與飛機資料快照組成 Executive 的原始輸入。
	/// @param context 本週期的時間間隔，單位為秒。
	/// @param aircraft 已完成 DCS 單位轉換的具型別資料快照。
	/// @return 包含飛行觀測、駕駛輸入、起落架與致動器回授的值物件。
	RawFlightControlInput make_pipeline_input(
		const SystemStepContext& context,
		const AircraftDataView& aircraft) const;

	FlightControlExecutive executive_;  // 持有控制、模式、濾波與排程狀態。
	FlightControlComputerDebugTelemetry debug_telemetry_;  // 發布診斷資料，不參與控制計算。
};

}
