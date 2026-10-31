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

struct TargetingData
{
	AirToAirSubmode submode = AirToAirSubmode::None;
	bool uncage_held = false;
	bool designation_requested = false; // 目標指定的操作意圖。
};

struct InfraredMissileData
{
	InfraredMissileReadiness readiness = InfraredMissileReadiness::Off;
	bool active = false;
	bool contact = false;
	bool locked = false;
	InfraredSeekerState seeker_state = InfraredSeekerState::Off;
	InfraredSeekerTone requested_tone = InfraredSeekerTone::Off;
	double seeker_azimuth_rad = 0.0;
	double seeker_elevation_rad = 0.0;
	double lock_range_m = 0.0;
};

struct StoresData
{
	int missile_count = 1;
	double gun_count = 523.0;
	StoresCountSource count_source = StoresCountSource::Estimate;
	std::optional<int> selected_station;
	std::optional<int> requested_station;
	std::uint64_t selection_request_revision = 0;
};

struct WeaponOperationData
{
	bool gun_requested = false;
	std::uint64_t gun_start_sequence = 0; // Identity of the accepted GunOn edge, including same-tick restarts.
	bool gun_firing_indication = false;
	bool pickle_requested = false;
};

/// @brief FireControl 及其內部模組唯一寫入；外部功能只讀指定分區。
struct FireControlData
{
	TargetingData targeting;
	WeaponOperationData weapon;
	InfraredMissileData infrared_missile;
};

}
