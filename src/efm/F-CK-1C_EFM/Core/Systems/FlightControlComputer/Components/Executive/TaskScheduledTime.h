#pragma once

#include <chrono>

namespace Core::Systems::Flcc
{
/// @brief 從本架飛機建立時的零時刻起算，以整數奈秒表示的模擬時間。
using TaskScheduledTime = std::chrono::nanoseconds;
}
