#pragma once

#include "../../../Contracts/AircraftData.h"

namespace Core::Systems::Dflcc
{
/// @brief DFLCC 目前可從 Pipeline 接收的具型別輸入。
struct FlightControlInputs
{
	FlightControlObservation observation; ///< 飛行狀態；單位遵循 Core 資料契約。
	PilotControlSignal pilot_control; ///< 三軸駕駛與配平輸入，使用正規化數值。
	LandingGearData landing_gear; ///< 起落架、煞車與接地資訊。
	FlightControlActuatorState actuator; ///< 控制面位置、速率及限制回授。
};

/// @brief DFLCC 對 Pipeline 發布的控制需求。
/// 舵面需求由 ControlLaws 計算。
struct FlightControlOutputs
{
	FlightControlActuatorCommand actuator_command; ///< 三軸舵面位置需求，範圍為 [-1, 1]。
};

/// @brief Pipeline 輸入與控制需求輸出的分區；由 DFLCC 管理 Pipeline 邊界。
/// 尚未實作：感測輸入選擇及有效性判斷。
struct InputOutputData
{
	FlightControlInputs input; ///< 外層最近一次 step 取得的輸入快照。
	FlightControlOutputs output; ///< 最近一次控制律運算的舵面需求。
};
}
