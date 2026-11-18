#pragma once
#include "../../../Contracts/CockpitContracts.h"
#include <string>
#include <string_view>

namespace Core::Systems::Mc
{
/// @brief 掛點保持、同型輪站與武器型別輪換的選擇意圖。
enum class MissileSelectionStep
{
	None,
	Station,
	WeaponType
};

/// @brief 一次掛點選擇及其 SMS 武器型別。
struct MissileSelection
{
	std::optional<int> station;
	std::string weapon_type;
};

/// @brief 依有效空對空庫存與目前型別，選擇同型掛點或下一型別。
MissileSelection select_air_to_air_station(const StoresInventorySnapshot& inventory,
                                           std::optional<int> current_station,
                                           std::string_view current_type,
                                           MissileSelectionStep step);
}
