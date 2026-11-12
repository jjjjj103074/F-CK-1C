#include "FireControl.h"

#include "Common/CommandValue.h"

namespace Core::Systems::Mc
{
FireControl::FireControl(const ModeData& mode,
                         const RadarSnapshot& radar,
                         const WeaponUseData& execution,
                         FireControlData& data,
                         RadarRequestQueue& radar_requests,
                         StoresCommandQueue& stores_commands)
    : targeting_(mode, radar, data.targeting, radar_requests),
      weapon_operation_(mode, execution, data.weapon, stores_commands)
{
}

void FireControl::handle(const Command& command)
{
	switch (command.id)
	{
	case CommandId::SetMissileUncage:
		weapon_operation_.handle(command);
		targeting_.handle(command);
		break;
	case CommandId::SetTmsUp:
	case CommandId::PressTmsDown:
	case CommandId::PressTmsLeft:
	case CommandId::PressTmsRight:
		targeting_.handle(command);
		break;
	case CommandId::SetWeaponRelease:
		if (Common::command_value_is_pressed(command.value_normalized))
			targeting_.before_weapon_release();
		weapon_operation_.handle(command);
		break;
	default:
		weapon_operation_.handle(command);
		break;
	}
}

void FireControl::on_operating_mode_selected(const OperatingModeSelected& event)
{
	targeting_.on_operating_mode_selected(event);
}

void FireControl::on_master_arm_selected(const MasterArmSelected& event)
{
	if (event.selected.master_arm != MasterArmMode::On)
		weapon_operation_.stop();
}

void FireControl::step(std::chrono::nanoseconds, double)
{
	if (weapon_operation_.step() == Internal::WeaponOperationEvent::ReleaseInputEnded)
		targeting_.on_weapon_release_ended();
	targeting_.step();
}
}
