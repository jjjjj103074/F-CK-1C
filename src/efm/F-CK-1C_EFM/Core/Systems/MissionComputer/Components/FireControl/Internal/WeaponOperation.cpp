#include "WeaponOperation.h"

#include "Common/CommandValue.h"

namespace Core::Systems::Mc::Internal
{
WeaponOperation::WeaponOperation(const ModeData& mode,
                                 const WeaponUseData& execution,
                                 WeaponOperationData& weapon,
                                 StoresCommandQueue& stores_commands)
    : mode_(mode),
      execution_(execution),
      weapon_(weapon),
      stores_commands_(stores_commands)
{
}

bool WeaponOperation::can_use_gun() const
{
	return mode_.master_arm == MasterArmMode::On && mode_.operating_mode == OperatingMode::Dogfight &&
	       !weapon_.uncage_held;
}

bool WeaponOperation::can_release_weapon() const
{
	return mode_.master_arm == MasterArmMode::On && mode_.air_to_air() && weapon_.uncage_held;
}

void WeaponOperation::handle(const Command& command)
{
	const bool pressed = Common::command_value_is_pressed(command.value_normalized);
	switch (command.id)
	{
	case CommandId::SetMissileUncage:
		weapon_.uncage_held = pressed;
		break;
	case CommandId::SetTriggerSecondStage:
		handle_gun(pressed);
		break;
	case CommandId::SetWeaponRelease:
		handle_release(pressed);
		break;
	default:
		break;
	}
}

void WeaponOperation::handle_gun(bool pressed)
{
	if (pressed)
	{
		if (can_use_gun())
			stores_commands_.submit(BeginWeaponUse{ WeaponUseKind::Gun });
	}
	else if (execution_.gun.request_id != 0)
		stores_commands_.submit(EndWeaponUse{ { WeaponUseKind::Gun, execution_.gun.request_id } });
}

void WeaponOperation::handle_release(bool pressed)
{
	if (pressed)
	{
		if (can_release_weapon())
			stores_commands_.submit(BeginWeaponUse{ WeaponUseKind::Release, LaunchPermission::Override });
	}
	else if (execution_.release.request_id != 0)
		stores_commands_.submit(EndWeaponUse{ { WeaponUseKind::Release, execution_.release.request_id } });
}

void WeaponOperation::stop()
{
	stores_commands_.submit(AbortWeaponUse{});
}

WeaponOperationEvent WeaponOperation::step()
{
	weapon_.gun_requested = execution_.gun.in_progress();
	weapon_.pickle_requested = execution_.release.in_progress();
	weapon_.gun_firing_indication = weapon_.gun_requested && mode_.master_arm == MasterArmMode::On &&
	                                mode_.operating_mode == OperatingMode::Dogfight;
	const auto& release = execution_.release;
	if (release.phase == WeaponUsePhase::StopScheduled && release.request_id != reported_release_end_)
	{
		reported_release_end_ = release.request_id;
		return WeaponOperationEvent::ReleaseInputEnded;
	}
	return WeaponOperationEvent::None;
}
}
