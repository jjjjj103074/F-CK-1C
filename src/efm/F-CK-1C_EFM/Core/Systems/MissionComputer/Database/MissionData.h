#pragma once

#include "../../../Contracts/CockpitContracts.h"

namespace Core::Systems::Mc
{
/// @brief MC 持有的共用模式分區；ModeManagement 唯一寫入，其他元件取得唯讀參照。
struct ModeData
{
	MasterArmMode master_arm = MasterArmMode::On; ///< 軟體主保險狀態。
	OperatingMode operating_mode = OperatingMode::Navigation;

	/// @brief 判斷目前模式是否為 DGFT/MSL。
	bool air_to_air() const noexcept
	{
		return operating_mode != OperatingMode::Navigation;
	}
};

/// @brief 由目標指定模組寫入，供武器使用與快照發布讀取。
using TargetDesignationData = TargetDesignationSnapshot;

struct WeaponOperationData
{
	bool uncage_held = false; ///< 駕駛的飛彈解籠操作狀態。
	bool gun_requested = false;
	bool gun_firing_indication = false;
	bool pickle_requested = false;
};

/// @brief FireControl 及其內部模組唯一寫入；外部功能只讀指定分區。
struct FireControlData
{
	AirToAirSubmode acquisition_mode = AirToAirSubmode::None; ///< FC 選擇的捕獲操作方式。
	TargetDesignationData target_designation;
	WeaponOperationData weapon;
};

}
