#include "WeaponOperation.h"

#include "Common/CommandValue.h"

#include <algorithm>

namespace Core::Systems::Mc::Internal
{
WeaponOperation::WeaponOperation(const MissionInputs& input,
                                 const ModeData& mode,
                                 WeaponOperationData& weapon,
                                 StoresCommandQueue& stores_commands)
    : input_(input),
      mode_(mode),
      weapon_(weapon),
      stores_commands_(stores_commands)
{
}

void WeaponOperation::handle(const Command& command)
{
	const bool pressed = Common::command_value_is_pressed(command.value_normalized);
	if (command.id == CommandId::SetMissileUncage)
	{
		weapon_.uncage_held = pressed;
		return;
	}
	if (command.id == CommandId::SetTriggerSecondStage)
	{
		if (!pressed)
		{
			gun_.request_release();
		}
		else if (input_.cockpit.mission_adapter.available && mode_.master_arm == MasterArmMode::On &&
		         mode_.operating_mode == OperatingMode::Dogfight && !weapon_.uncage_held)
		{
			weapon_.gun_start_ticket = stores_commands_.submit(StoresWeaponAction{ MissionActionKind::GunOn });
			gun_.begin(weapon_.gun_start_ticket);
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
	if (input_.cockpit.mission_adapter.available && mode_.master_arm == MasterArmMode::On && mode_.air_to_air() &&
	    weapon_.uncage_held)
	{
		stores_commands_.submit(StoresWeaponAction{ MissionActionKind::LaunchPermissionOverride });
		pickle_.begin(stores_commands_.submit(StoresWeaponAction{ MissionActionKind::PickleOn }));
		weapon_.pickle_requested = true;
	}
}

void WeaponOperation::finish_gun()
{
	stores_commands_.submit(StoresWeaponAction{ MissionActionKind::GunOff });
	gun_.clear();
	weapon_.gun_requested = false;
	weapon_.gun_start_ticket = 0;
}

void WeaponOperation::finish_pickle()
{
	stores_commands_.submit(StoresWeaponAction{ MissionActionKind::PickleOff });
	pickle_.clear();
	weapon_.pickle_requested = false;
}

void WeaponOperation::stop()
{
	stores_commands_.submit(CancelWeaponStarts{});
	// 主保險 OFF/SIM 時立即取消程序，涵蓋等待 Lua 確認的動作。
	if (gun_.active())
		finish_gun();
	if (pickle_.active())
		finish_pickle();
}

WeaponOperationEvent WeaponOperation::step(std::chrono::nanoseconds time, double)
{
	time = (std::max)(time, input_.observed_at);
	if (gun_.release_due(time, stores_commands_))
		finish_gun();
	const bool release_ended = pickle_.release_due(time, stores_commands_);
	if (release_ended)
		finish_pickle();
	weapon_.gun_firing_indication = weapon_.gun_requested && mode_.master_arm == MasterArmMode::On &&
	                                mode_.operating_mode == OperatingMode::Dogfight;
	return release_ended ? WeaponOperationEvent::ReleaseInputEnded : WeaponOperationEvent::None;
}
}
