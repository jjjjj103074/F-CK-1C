#pragma once

#include "../../Contracts/AircraftData.h"
#include "../../Contracts/CockpitContracts.h"
#include "../../Contracts/Commands.h"

#include <functional>
#include <vector>

namespace Core::Systems
{
/// @brief 一次 FLCC 運算所需的完整輸入邊界。
/// 所有數值都已由 Core 轉成具型別資料與約定單位，不含 DCS 原始資料。
struct FlightControlComputerStepInput
{
	double dt_s = 0.0;  // 本次排程週期的時間間隔，單位為秒。
	FlightControlObservation observation;  // 飛行狀態觀測值。
	PilotControlSignal pilot_control;  // 駕駛桿、方向舵與配平輸入。
	ThrottleLeverSignal throttle_levers;  // 左右油門桿正規化位置。
	LandingGearData landing_gear;  // 起落架、輪煞車與接地狀態。
	FlightControlActuatorState actuator;  // 三軸致動器位置與限制回授。
};

/// @brief 一次 FLCC 運算完成後應產生的完整輸出邊界。
struct FlightControlComputerResult
{
	FlightControlActuatorCommand actuator_command;  // 三軸控制面需求，單位為弧度。
	EngineThrottleCommand engine_throttle_command;  // 左右引擎正規化油門命令。
	FlightControlComputerSnapshot diagnostics;  // FLCC 狀態與診斷快照。
	AutomaticFlightControlSnapshot automatic_flight_control;  // 自動飛行狀態快照。
};

/// @brief Executive 提供給外層 System 登記的指令綁定。
struct FlightControlCommandBinding
{
	CommandId id = CommandId::NoOp;  // Pipeline 傳入的指令識別碼。
	std::function<void(const Command&)> deliver;  // 將指令交給未來子模組的函式。
};

/// @brief FLCC 內部運算的唯一協調者。
/// 目前只保留介面骨架；控制律、模式、濾波及其他運算將以測試驅動方式重建。
class FlightControlExecutive final
{
public:
	FlightControlExecutive() = default;

	/// @brief 取得目前可由 Pipeline 傳入 FLCC 的指令綁定。
	/// @return 本實例持有的綁定清單；目前尚未建立子模組，因此清單為空。
	const std::vector<FlightControlCommandBinding>& command_bindings() const noexcept;

	/// @brief 執行一次 FLCC 內部運算。
	/// @param input 本週期的完整具型別輸入。
	/// @return 未來會回傳本週期的完整輸出；目前此函式一定拋出例外。
	FlightControlComputerResult update(
		const FlightControlComputerStepInput& input);

private:
	const std::vector<FlightControlCommandBinding> command_bindings_;  // 未來由子模組彙整的指令綁定。
};
}
