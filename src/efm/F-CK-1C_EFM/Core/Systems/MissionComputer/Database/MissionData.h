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
	Aim9Readiness readiness = Aim9Readiness::Off;
};

struct StoresData
{
	int missile_count = 1;
	bool live_missile_count = false;
};

struct WeaponControlData
{
	bool gun_requested = false;
	bool gun_firing_indication = false;
	bool pickle_requested = false;
};
}
