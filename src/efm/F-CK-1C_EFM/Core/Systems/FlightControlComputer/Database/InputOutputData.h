#pragma once

#include "../../../Contracts/AircraftData.h"

namespace Core::Systems::Flcc
{
/// @brief FLCC 目前可從 Pipeline 接收的具型別輸入。
struct FlightControlInputs
{
	FlightControlObservation observation; ///< 飛行狀態；單位遵循 Core 資料契約。
	PilotControlSignal pilot_control; ///< 三軸駕駛與配平輸入，使用正規化數值。
	ThrottleLeverSignal throttle_levers; ///< 左右油門桿位置，範圍為 0 到 1。
	LandingGearData landing_gear; ///< 起落架、煞車與接地資訊。
	FlightControlActuatorState actuator; ///< 控制面位置、速率及限制回授。
};

/// @brief FLCC 對 Pipeline 發布的控制需求。
/// 尚未實作：控制需求的計算；骨架保留中立控制面與建構時的油門初值。
struct FlightControlOutputs
{
	FlightControlActuatorCommand actuator_command; ///< 三軸控制面需求，單位為弧度。
	EngineThrottleCommand engine_throttle_command; ///< 左右引擎需求，使用正規化數值。
};

/// @brief 輸入、輸出與未來選擇後資料的分區；由 FLCC 管理 Pipeline 邊界。
/// 尚未實作：感測輸入選擇及有效性判斷。
struct InputOutputData
{
	FlightControlInputs input; ///< 外層最近一次 step 取得的輸入快照。
	FlightControlOutputs output; ///< 本階段僅含初始化的輸出值。
};
}
