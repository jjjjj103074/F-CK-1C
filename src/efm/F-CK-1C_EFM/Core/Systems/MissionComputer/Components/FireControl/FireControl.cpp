#include "FireControl.h"

namespace Core::Systems::Mc
{
FireControl::FireControl(const MissionInputs& input,
                         const ModeData& mode,
                         const StoresData& stores,
                         FireControlData& data,
                         ExternalActionQueue& actions,
                         StoresManagement& stores_management)
    : targeting_(input, mode, stores, data.targeting, actions, stores_management),
      weapon_operation_(input, mode, data.targeting, data.weapon, actions, targeting_, stores_management),
      infrared_missile_support_(input, mode, stores, data.targeting, data.infrared_missile)
{
}

void FireControl::handle(const Command& command)
{
	switch (command.id)
	{
	case CommandId::SetMissileUncage:
	case CommandId::SetTmsUp:
	case CommandId::PressTmsDown:
	case CommandId::PressTmsLeft:
	case CommandId::PressTmsRight:
		targeting_.handle(command);
		break;
	default:
		weapon_operation_.handle(command);
		break;
	}
}

void FireControl::on_operating_mode_selected(const OperatingModeSelected& event)
{
	targeting_.select_mode(event.cycle_weapon);
}

void FireControl::on_master_arm_selected(const MasterArmSelected& event)
{
	if (event.selected.master_arm != MasterArmMode::On)
		weapon_operation_.stop();
}

void FireControl::step(std::chrono::nanoseconds time, double dt_s)
{
	// 保留原本同時到期的順序：先完成射擊／投放程序，再處理目標操作計時。
	weapon_operation_.step(time, dt_s);
	targeting_.step(time, dt_s);
	infrared_missile_support_.step(time, dt_s);
}
}
