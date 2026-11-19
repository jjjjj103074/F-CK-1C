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

void ModeManagement::publish_selection(const ModeData& previous)
{
	events_.operating_mode_selected(OperatingModeSelected{ previous, mode_ });
}

void ModeManagement::select_master_mode(MasterMode mode)
{
	if (mode_.override_mode != OverrideMode::None)
		return;
	const ModeData previous = mode_;
	mode_.master_mode = mode_.master_mode == mode ? MasterMode::Navigation : mode;
	publish_selection(previous);
}

void ModeManagement::select_override(OverrideMode mode)
{
	if (mode_.override_mode == mode)
		return;
	const ModeData previous = mode_;
	mode_.override_mode = mode;
	publish_selection(previous);
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
		// 主保險事件依序通知火控停止與 CMS 中止。
		select_master_arm(command.id == CommandId::SetMasterArmOff ? MasterArmMode::Off : MasterArmMode::Simulation);
		break;

	case CommandId::SetMasterArmOn: // Set Master Arm to On
		select_master_arm(MasterArmMode::On);
		break;

	case CommandId::SelectNavigationMode: // Select Navigation Mode
		select_master_mode(MasterMode::Navigation);
		break;
	case CommandId::SelectAirToAirMode:
		select_master_mode(MasterMode::AirToAir);
		break;
	case CommandId::SelectAirToGroundMode:
		select_master_mode(MasterMode::AirToGround);
		break;
	case CommandId::SelectOverrideCenter:
		select_override(OverrideMode::None);
		break;

	case CommandId::SelectDogfightMode: // Select Dogfight Mode
	case CommandId::SelectMissileOverride: // Select Missile Override Mode
		select_override(command.id == CommandId::SelectDogfightMode ? OverrideMode::Dogfight : OverrideMode::Missile);
		break;

	default:
		break;
	}
}
}
