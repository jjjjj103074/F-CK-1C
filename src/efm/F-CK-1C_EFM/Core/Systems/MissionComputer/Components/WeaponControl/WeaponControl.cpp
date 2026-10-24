#include "WeaponControl.h"

#include "Common/CommandValue.h"

#include <algorithm>

namespace Core::Systems::Mc
{
WeaponControl::WeaponControl(const MissionInputs& input,
                             const ModeData& mode,
                             const SensorData& sensor,
                             WeaponControlData& weapon,
                             ExternalActionQueue& actions,
                             SensorCoordination& sensors,
                             StoresManagement& stores)
    : input_(input),
      mode_(mode),
      sensor_(sensor),
      weapon_(weapon),
      actions_(actions),
      sensors_(sensors),
      stores_(stores)
{
}

void WeaponControl::handle(const Command& command)
{
	const bool pressed = Common::command_value_is_pressed(command.value_normalized);
	if (command.id == CommandId::SetTriggerSecondStage)
	{
		if (!pressed)
		{
			gun_.request_release();
		}
		else if (input_.cockpit.mission_adapter.available && mode_.master_arm == MasterArmMode::On &&
		         mode_.fire_control == FireControlMode::Dogfight && !sensor_.uncage_held)
		{
			gun_.begin(actions_.append(MissionActionKind::GunOn));
			weapon_.gun_requested = true;
		}
		return;
	}
	if (command.id != CommandId::SetWeaponRelease)
		return;
	if (!pressed)
	{
		pickle_.request_release();
		return;
	}
	sensors_.prepare_release();
	if (input_.cockpit.mission_adapter.available && mode_.master_arm == MasterArmMode::On && mode_.air_to_air() &&
	    sensor_.uncage_held)
	{
		actions_.append(MissionActionKind::LaunchPermissionOverride);
		pickle_.begin(actions_.append(MissionActionKind::PickleOn));
		weapon_.pickle_requested = true;
	}
}

void WeaponControl::finish_gun()
{
	actions_.append(MissionActionKind::GunOff);
	gun_.clear();
	weapon_.gun_requested = false;
}

void WeaponControl::finish_pickle(bool consume)
{
	actions_.append(MissionActionKind::PickleOff);
	pickle_.clear();
	weapon_.pickle_requested = false;
	if (consume)
	{
		stores_.record_missile_release();
		sensors_.unlock();
	}
}

void WeaponControl::stop()
{
	// Master OFF/SIM cancels immediately, including a leading edge not yet acknowledged by Lua.
	if (gun_.active())
		finish_gun();
	if (pickle_.active())
		finish_pickle(false);
}

void WeaponControl::step(std::chrono::nanoseconds time, double)
{
	time = (std::max)(time, input_.observed_at);
	if (gun_.release_due(time, actions_))
		finish_gun();
	if (pickle_.release_due(time, actions_))
		finish_pickle(true);
	weapon_.gun_firing_indication = weapon_.gun_requested && mode_.master_arm == MasterArmMode::On &&
	                                mode_.fire_control == FireControlMode::Dogfight;
}
}
