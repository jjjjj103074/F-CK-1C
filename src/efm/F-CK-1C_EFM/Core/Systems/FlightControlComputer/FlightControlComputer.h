#pragma once

#include "FlightControlExecutive.h"
#include "../System.h"

namespace Core::Systems
{
/// @brief FLCC 與 SystemPipeline 之間的生命週期及資料邊界轉接器。
/// 此類別只宣告輸入輸出、轉換本週期資料，並把運算交給 Executive。
class FlightControlComputer final : public System
{
public:
	/// @brief 建立一架飛機的 FLCC 邊界轉接器。
	/// @param initial_throttle_levers 首次更新前的左右油門桿位置，範圍為 0 到 1。
	explicit FlightControlComputer(
		const ThrottleLeverSignal& initial_throttle_levers);

	/// @brief 向 Pipeline 宣告更新率、輸入、初始輸出與指令處理器。
	/// @param setup Pipeline 提供的系統宣告介面；每架飛機建立時呼叫一次。
	void setup(SystemSetup& setup) override;

	/// @brief 讀取本週期輸入並交由 Executive 運算。
	/// @param context 本週期的排程時間與時間間隔。
	/// @param aircraft 本週期可讀取的具型別飛機資料快照。
	/// @param result 本週期具型別輸出的發布介面。
	void step(
		const SystemStepContext& context,
		const AircraftDataView& aircraft,
		SystemResult& result) override;

private:
	/// @brief 將 Pipeline 資料組成 Executive 的單次運算輸入。
	/// @param context 本週期的排程資訊，時間間隔單位為秒。
	/// @param aircraft 已完成 DCS 座標與單位轉換的具型別資料。
	/// @return 不含 Pipeline 資料鍵的 FLCC 輸入值物件。
	FlightControlComputerStepInput make_step_input(
		const SystemStepContext& context,
		const AircraftDataView& aircraft) const;

	const ThrottleLeverSignal initial_throttle_levers_;  // Pipeline 建立完成前使用的油門初值。
	FlightControlExecutive executive_;  // 持有未來的 FLCC 運算流程與跨週期狀態。
};
}
