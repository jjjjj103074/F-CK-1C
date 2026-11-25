#include "FireControl.h"

#include "Common/CommandValue.h"

namespace Core::Systems::Mc
{
FireControl::FireControl(const ModeData& mode,
                         const RadarSnapshot& radar,
                         const SelectedWeaponState& selected,
                         const WeaponUseData& execution,
                         FireControlData& data,
                         RadarRequestQueue& radar_requests,
                         StoresCommandQueue& stores_commands)
    : targeting_(mode, radar, data.targeting, radar_requests),
      weapon_operation_(mode, radar, selected, execution, data.weapon, stores_commands)
{
}

void FireControl::handle(const Command& command, std::chrono::nanoseconds input_time)
{
	switch (command.id)
	{
	case CommandId::ToggleMissileUncage:
	case CommandId::SetMissileUncage:
		weapon_operation_.handle(command, input_time);
		break;
	case CommandId::SetTmsUp:
	case CommandId::PressTmsDown:
	case CommandId::PressTmsLeft:
	case CommandId::PressTmsRight:
		targeting_.handle(command, input_time);
		break;
	case CommandId::SetWeaponRelease:
		if (Common::command_value_is_pressed(command.value_normalized))
			targeting_.before_weapon_release();
		weapon_operation_.handle(command, input_time);
		break;
	default:
		weapon_operation_.handle(command, input_time);
		break;
	}
}

void FireControl::on_operating_mode_selected(const OperatingModeSelected& event)
{
	// 結束先前配置的武器使用，再套用本次模式的目標取得設定。
	if (event.previous.operating_mode() != event.selected.operating_mode())
		weapon_operation_.on_mode_changed();
	targeting_.on_operating_mode_selected(event);
}

void FireControl::on_master_arm_selected(const MasterArmSelected& event)
{
	if (event.selected.master_arm != MasterArmMode::On)
		weapon_operation_.stop();
	if (event.selected.master_arm == MasterArmMode::Off)
		weapon_operation_.handle({ CommandId::SetMissileUncage, 0.0 }, {});
}

void FireControl::step(std::chrono::nanoseconds time, double)
{
	weapon_operation_.step(time);
	targeting_.step(time);
}
}
