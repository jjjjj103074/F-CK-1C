#pragma once

#include "../../../Contracts/CockpitContracts.h"
#include "WeaponUseData.h"

namespace Core::Systems::Mc
{
/// @brief MC 持有的掛載資料；SM 統一寫入，使用者唯讀庫存、彈量、選站、執行與 IR 觀測。
struct StoresData
{
	StoresInventorySnapshot inventory;
	IrSeekerObservation ir_seeker; ///< 目前全域武器介面的 IR 來源觀測。
	GunAmmunitionState gun_ammunition;
	StationSelectionState selection;
	WeaponUseData weapon_use; ///< SM 寫入的使用執行結果，供火控追蹤。
};
}
