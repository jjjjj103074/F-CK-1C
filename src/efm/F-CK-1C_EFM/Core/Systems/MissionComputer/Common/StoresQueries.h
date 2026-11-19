#pragma once

#include "../../../Contracts/CockpitContracts.h"

#include <string_view>
#include <vector>

namespace Core::Systems::Mc
{
/// @brief 唯讀飛彈查詢條件；空條件涵蓋全部有彈飛彈掛點。
struct MissileQuery
{
	std::optional<WeaponRole> role;
	std::optional<std::string_view> weapon_identifier;
};

struct MissileCount
{
	int count = 0;
	bool complete = true; ///< 所有掛點均有有效觀測，且分類足以回答本次查詢。
};

/// @brief 依掛點順序回傳符合條件且具有有效觀測的飛彈掛點。
std::vector<int> find_missile_stations(const StoresInventorySnapshot& inventory, const MissileQuery& query = {});
/// @brief 加總有效觀測中的飛彈數量，並回報資料完整性。
MissileCount count_missiles(const StoresInventorySnapshot& inventory, const MissileQuery& query = {});
}
