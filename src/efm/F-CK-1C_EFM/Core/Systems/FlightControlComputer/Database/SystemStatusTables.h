#pragma once

#include "../../../Contracts/CockpitContracts.h"

namespace Core::Systems::Flcc
{
/// @brief FLCC 狀態分區及對外快照；所有讀取者共用同一實例。
/// 尚未實作：監控、故障、啟動狀態；兩種快照均維持 available 為 false。
struct SystemStatusTables
{
	FlightControlComputerSnapshot flight_control_computer; ///< 飛控電腦的對外狀態。
	AutomaticFlightControlSnapshot automatic_flight_control; ///< 自動飛行的對外狀態。
};
}
