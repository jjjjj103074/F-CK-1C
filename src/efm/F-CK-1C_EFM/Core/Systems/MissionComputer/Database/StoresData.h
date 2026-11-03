#pragma once

#include "../../../Contracts/CockpitContracts.h"

namespace Core::Systems::Mc
{
/// @brief MC 持有的掛載分區；StoresManagement 唯一寫入，其他使用者取得唯讀參照。
struct StoresData
{
	StoresInventorySnapshot inventory;
	// 既有座艙介面使用的空對空總數與彈量來源。
	int missile_count = 1;
	GunAmmunitionState gun_ammunition;
	StoresCountSource count_source = StoresCountSource::Estimate;
	std::optional<int> selected_station; ///< Lua 最近確認執行的選站。
	std::optional<int> requested_station;
	std::uint64_t selection_request_revision = 0;
};
}
