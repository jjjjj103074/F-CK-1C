#pragma once

#include "../../../Contracts/CockpitContracts.h"

namespace Core::Systems::Mc
{
struct ModeData
{
	MasterArmMode master_arm = MasterArmMode::On;
	// 保留原有三態與 Master Arm 耦合。後續再設計 NAV／A-A／A-G、超控與恢復。
	FireControlMode fire_control = FireControlMode::Navigation;

	bool air_to_air() const noexcept
	{
		return fire_control != FireControlMode::Navigation;
	}
};

struct TargetingData
{
	AirToAirSubmode submode = AirToAirSubmode::None;
	bool uncage_held = false;
	bool designation_requested = false; // 操作意圖，不代表雷達／飛彈已實際鎖定。
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
