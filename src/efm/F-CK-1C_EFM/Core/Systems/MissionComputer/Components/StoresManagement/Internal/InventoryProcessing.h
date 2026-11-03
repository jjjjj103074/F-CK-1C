#pragma once

#include "../../../../../Contracts/CockpitContracts.h"

namespace Core::Systems::Mc::Internal
{
/// @brief 計算單一掛點的新觀測狀態與掛載版本，回傳內容或有效性是否改變。
bool update_station(StoreStationState& station,
                    const WeaponStationSample& sample,
                    const ObservationStatus& observation,
                    std::chrono::nanoseconds time,
                    bool rearmed);
}
