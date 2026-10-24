#include "DisplayManagement.h"

#include "Common/CommandValue.h"

namespace Core::Systems::Mc
{
DisplayManagement::DisplayManagement(const ModeData& mode,
                                     const SensorData& sensor,
                                     const StoresData& stores,
                                     const WeaponControlData& weapon,
                                     const Aim9Data& aim9,
                                     WeaponDisplayData& display)
    : mode_(mode),
      sensor_(sensor),
      stores_(stores),
      weapon_(weapon),
      aim9_(aim9),
      display_(display)
{
}

void DisplayManagement::observe(const Command& command)
{
	const bool pressed = Common::command_value_is_pressed(command.value_normalized);
	if (command.id == CommandId::SetTriggerSecondStage)
	{
		display_.trigger_held = pressed;
		return;
	}
	if (!pressed)
		return;
	switch (command.id)
	{
	case CommandId::SetMasterArmOn:
	case CommandId::SetMasterArmOff:
	case CommandId::SetMasterArmSim:
	case CommandId::SelectNavigationMode:
		display_.preferred_class = WeaponClass::Gun;
		break;
	case CommandId::SelectDogfightMode:
	case CommandId::SelectMissileOverride:
	case CommandId::SetMissileUncage:
	case CommandId::SetWeaponRelease:
		display_.preferred_class = WeaponClass::AirToAirMissile;
		break;
	default:
		break;
	}
}

void DisplayManagement::step(std::chrono::nanoseconds, double)
{
	if (mode_.air_to_air() || sensor_.uncage_held || aim9_.contact || aim9_.locked ||
	    aim9_.readiness != Aim9Readiness::Off)
		display_.weapon_class = WeaponClass::AirToAirMissile;
	else if (weapon_.gun_firing_indication || display_.trigger_held || display_.preferred_class == WeaponClass::Gun)
		display_.weapon_class = WeaponClass::Gun;
	else
		display_.weapon_class = display_.preferred_class;
	display_.quantity = display_.weapon_class == WeaponClass::Gun ? stores_.gun_count : stores_.missile_count;
}
}
