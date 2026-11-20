#pragma once

#include "../../../Contracts/CockpitContracts.h"

namespace Core::Systems::DisplayPresentation
{
/// @brief 依雷達來源、顯示配置與模擬時間產生前視框及雷達圖。
RadarDisplaySnapshot radar(const RadarSnapshot& source, OperatingMode mode, std::chrono::nanoseconds time);
}
