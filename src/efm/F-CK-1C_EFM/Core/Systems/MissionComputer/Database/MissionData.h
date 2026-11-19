#pragma once

#include "../../../Contracts/CockpitContracts.h"

namespace Core::Systems::Mc
{
/// @brief MC 持有的共用模式分區；ModeManagement 唯一寫入，其他元件取得唯讀參照。
struct ModeData
{
	MasterArmMode master_arm = MasterArmMode::On; ///< 軟體主保險狀態。
	MasterMode master_mode = MasterMode::Navigation; ///< 超控前後持續保存的一般主模式。
	OverrideMode override_mode = OverrideMode::None; ///< 油門超控開關位置。

	/// @brief 依超控位置及保存的主模式取得目前生效的操作配置。
	OperatingMode operating_mode() const noexcept
	{
		if (override_mode == OverrideMode::Dogfight)
			return OperatingMode::Dogfight;
		if (override_mode == OverrideMode::Missile)
			return OperatingMode::MissileOverride;
		switch (master_mode)
		{
		case MasterMode::AirToAir:
			return OperatingMode::AirToAir;
		case MasterMode::AirToGround:
			return OperatingMode::AirToGround;
		default:
			return OperatingMode::Navigation;
		}
	}

	/// @brief 判斷一般 A-A 或空戰超控是否生效。
	bool air_to_air() const noexcept
	{
		return is_air_to_air_mode(operating_mode());
	}
};

/// @brief Targeting 唯一寫入的目標取得策略及指定資料。
struct TargetingData
{
	TargetAcquisitionMode acquisition_mode = TargetAcquisitionMode::None;
	RadarSearchMode search_mode = RadarSearchMode::RangeWhileSearch;
	TargetDesignationSnapshot designation;
	std::optional<RadarRequest> last_tms_request;
};

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
	TargetingData targeting;
	WeaponOperationData weapon;
};

}
