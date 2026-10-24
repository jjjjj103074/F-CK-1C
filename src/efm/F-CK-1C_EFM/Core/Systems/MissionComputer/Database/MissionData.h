#pragma once

#include "../../../Contracts/CockpitContracts.h"

namespace Core::Systems::Mc
{
struct ModeData
{
	MasterArmMode master_arm = MasterArmMode::On;
	FireControlMode fire_control = FireControlMode::Navigation;

	bool air_to_air() const noexcept
	{
		return fire_control != FireControlMode::Navigation;
	}
};

struct SensorData
{
	AirToAirSubmode submode = AirToAirSubmode::None;
	bool uncage_held = false;
	bool target_designated = false;
};

struct Aim9Data
{
	Aim9Readiness readiness = Aim9Readiness::Off;
	bool active = false;
	bool contact = false;
	bool locked = false;
	Aim9SeekerState seeker_state = Aim9SeekerState::Off;
	Aim9Tone requested_tone = Aim9Tone::Off;
	double seeker_azimuth_rad = 0.0;
	double seeker_elevation_rad = 0.0;
	double lock_range_m = 0.0;
};

struct WeaponDisplayData
{
	WeaponClass preferred_class = WeaponClass::Gun;
	bool trigger_held = false;
	WeaponClass weapon_class = WeaponClass::Gun;
	double quantity = 0.0;
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

struct WeaponControlData
{
	bool gun_requested = false;
	std::uint64_t gun_start_sequence = 0; // Identity of the accepted GunOn edge, including same-tick restarts.
	bool gun_firing_indication = false;
	bool pickle_requested = false;
};
}
