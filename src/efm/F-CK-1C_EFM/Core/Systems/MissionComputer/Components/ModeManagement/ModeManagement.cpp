#include "ModeManagement.h"

#include "Common/CommandValue.h"

namespace Core::Systems::Mc
{
ModeManagement::ModeManagement(ModeData& mode, ModeEvents& events)
    : mode_(mode),
      events_(events)
{
}

void ModeManagement::select_master_arm(MasterArmMode mode)
{
	// 每次選擇都發布事件，包括重選相同值。
	const ModeData previous = mode_;
	mode_.master_arm = mode;
	events_.master_arm_selected(MasterArmSelected{ previous, mode_ });
}

void ModeManagement::select_operating_mode(OperatingMode mode, bool cycle_weapon)
{
	const ModeData previous = mode_;
	mode_.operating_mode = mode;
	events_.operating_mode_selected(OperatingModeSelected{ previous, mode_, cycle_weapon });
}

void ModeManagement::handle(const Command& command)
{
	// 只處理按下事件。
	if (!Common::command_value_is_pressed(command.value_normalized))
		return;

	// 依照command.id運行指令
	switch (command.id)
	{
	case CommandId::SetMasterArmOff: // Set Master Arm to Off
	case CommandId::SetMasterArmSim: // Set Master Arm to Simulation
		// 同步完成火控停止與 CMS 中止後才選 NAV，保留原有外部動作順序。
		select_master_arm(command.id == CommandId::SetMasterArmOff ? MasterArmMode::Off : MasterArmMode::Simulation);
		select_operating_mode(OperatingMode::Navigation, false);
		break;

	case CommandId::SetMasterArmOn: // Set Master Arm to On
		// 主保險切至 ON，模式切至 NAV。
		select_master_arm(MasterArmMode::On);
		select_operating_mode(OperatingMode::Navigation, false);
		break;

	case CommandId::SelectNavigationMode: // Select Navigation Mode
		select_operating_mode(OperatingMode::Navigation, false);
		break;

	case CommandId::SelectDogfightMode: // Select Dogfight Mode
	case CommandId::SelectMissileOverride: // Select Missile Override Mode
		// 選擇 DGFT/MSL 時切至 ON，並要求武器循環。
		select_master_arm(MasterArmMode::On);
		select_operating_mode(command.id == CommandId::SelectDogfightMode ? OperatingMode::Dogfight
		                                                                  : OperatingMode::MissileOverride,
		                      true);
		break;

	default:
		break;
	}
}
}
