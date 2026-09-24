#pragma once

#include "../../../Contracts/Commands.h"

#include <functional>

namespace Core::Systems
{
/// @brief 一架飛機可接收的一種指令，以及接收後的交付方式。
/// 此資料不依賴 SystemPipeline；處理函式只接收 Core 指令。
struct FlightControlCommandBinding
{
	CommandId id;  // 供 Pipeline 登記與查找的指令識別碼。
	std::function<void(const Command&)> deliver;  // 將原始指令交給持有狀態的模組。
};
}
