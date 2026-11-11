#pragma once

#include "../../../Contracts/CockpitContracts.h"
#include "WeaponUseData.h"

namespace Core::Systems::Mc
{
/// @brief MC 持有的掛載資料；SM 統一寫入，使用者讀取唯讀庫存、彈量及選站狀態。
struct StoresData
{
	StoresInventorySnapshot inventory;
	GunAmmunitionState gun_ammunition;
	StationSelectionState selection;
	WeaponUseData weapon_use; ///< SM 寫入的使用執行結果，供火控追蹤。
};
}
