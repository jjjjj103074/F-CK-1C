#pragma once

#include "../../../StoresEvents.h"

namespace Core::Systems::Mc::Internal
{
/// @brief 接收新批次並更新固定掛點，回報掛載、重新掛彈及有效性變化。
InventoryChanged update_inventory(StoresInventorySnapshot& inventory,
                                  const WeaponStationObservation& observation,
                                  std::chrono::nanoseconds time);
/// @brief 接收原生機砲彈量，回報數量或有效性變化。
bool update_gun_ammunition(GunAmmunitionState& gun,
                           const GunAmmunitionObservation& observation,
                           std::chrono::nanoseconds time);
}
