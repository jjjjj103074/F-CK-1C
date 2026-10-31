#include "WeaponOperation.h"

#include "Common/CommandValue.h"

#include <algorithm>

namespace Core::Systems::Mc::Internal
{
WeaponOperation::WeaponOperation(const MissionInputs& input,
                                 const ModeData& mode,
                                 const TargetingData& targeting,
                                 WeaponOperationData& weapon,
                                 ExternalActionQueue& actions,
                                 Targeting& targeting_control,
                                 StoresManagement& stores)
    : input_(input),
      mode_(mode),
      targeting_(targeting),
      weapon_(weapon),
      actions_(actions),
      targeting_control_(targeting_control),
      stores_(stores)
{
}

void WeaponOperation::handle(const Command& command)
{
	const bool pressed = Common::command_value_is_pressed(command.value_normalized);
	if (command.id == CommandId::SetTriggerSecondStage)
	{
		if (!pressed)
		{
			gun_.request_release();
		}
		else if (input_.cockpit.mission_adapter.available && mode_.master_arm == MasterArmMode::On &&
		         mode_.operating_mode == OperatingMode::Dogfight && !targeting_.uncage_held)
		{
			weapon_.gun_start_sequence = actions_.append(MissionActionKind::GunOn);
			gun_.begin(weapon_.gun_start_sequence);
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
	targeting_control_.prepare_release();
	if (input_.cockpit.mission_adapter.available && mode_.master_arm == MasterArmMode::On && mode_.air_to_air() &&
	    targeting_.uncage_held)
	{
		actions_.append(MissionActionKind::LaunchPermissionOverride);
		pickle_.begin(actions_.append(MissionActionKind::PickleOn));
		weapon_.pickle_requested = true;
	}
}

void WeaponOperation::finish_gun()
{
	actions_.append(MissionActionKind::GunOff);
	gun_.clear();
	weapon_.gun_requested = false;
	weapon_.gun_start_sequence = 0;
}

void WeaponOperation::finish_pickle(bool consume)
{
	actions_.append(MissionActionKind::PickleOff);
	pickle_.clear();
	weapon_.pickle_requested = false;
	if (consume)
	{
		stores_.record_missile_release();
		targeting_control_.unlock();
	}
}

void WeaponOperation::stop()
{
	// 主保險 OFF/SIM 時立即取消程序，涵蓋等待 Lua 確認的動作。
	if (gun_.active())
		finish_gun();
	if (pickle_.active())
		finish_pickle(false);
}

void WeaponOperation::step(std::chrono::nanoseconds time, double)
{
	time = (std::max)(time, input_.observed_at);
	if (gun_.release_due(time, actions_))
		finish_gun();
	if (pickle_.release_due(time, actions_))
		finish_pickle(true);
	weapon_.gun_firing_indication = weapon_.gun_requested && mode_.master_arm == MasterArmMode::On &&
	                                mode_.operating_mode == OperatingMode::Dogfight;
}
}
