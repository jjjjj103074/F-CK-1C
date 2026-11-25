#pragma once
#include "StoresQueries.h"
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

/// @brief 判斷 AIM-9 家族，供選擇與顯示共用群組定義。
bool is_sidewinder(const WeaponDefinitionRef& weapon) noexcept;
/// @brief 判斷目前型號的雷達視線指向支援。
bool supports_radar_cue(const WeaponDefinitionRef& weapon) noexcept;
/// @brief 統計目前所選 SMS 群組的全機剩餘數量。
MissileCount count_selected_missile_group(const StoresInventorySnapshot& inventory, int station);

/// @brief 依有效空對空庫存與目前型別，選擇同型掛點或下一型別。
MissileSelection select_air_to_air_station(const StoresInventorySnapshot& inventory,
                                           std::optional<int> current_station,
                                           std::string_view current_type,
                                           MissileSelectionStep step);
}
