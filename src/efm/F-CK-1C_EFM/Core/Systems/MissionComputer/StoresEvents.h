#pragma once

#include "../../Contracts/CockpitContracts.h"

#include <bitset>
#include <sigslot/signal.hpp>

namespace Core::Systems::Mc
{
/// @brief 已完成寫入的掛載變化；接收者依版本讀取共用庫存。
struct InventoryChanged
{
	std::uint64_t revision = 0;
	std::bitset<kWeaponStationCount> changed_stations;
	bool rearmed = false;
};

/// @brief 每台 MC 持有的掛載同步通知通道。
struct StoresEvents
{
	sigslot::signal_st<const InventoryChanged&> inventory_changed;
};
}
