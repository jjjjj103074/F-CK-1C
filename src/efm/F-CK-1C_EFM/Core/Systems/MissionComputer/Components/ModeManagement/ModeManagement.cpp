#include "ModeManagement.h"

#include "Common/CommandValue.h"

namespace Core::Systems::Mc
{
ModeManagement::ModeManagement(ModeData& mode, FireControl& fire_control, ExternalActionQueue& actions)
    : mode_(mode),
      fire_control_(fire_control),
      actions_(actions)
{
}

void ModeManagement::handle(const Command& command)
{
	if (!Common::command_value_is_pressed(command.value_normalized))
		return;
	bool cycle_weapon = false;
	switch (command.id)
	{
	case CommandId::SetMasterArmOff:
	case CommandId::SetMasterArmSim:
		mode_.master_arm = command.id == CommandId::SetMasterArmOff ? MasterArmMode::Off : MasterArmMode::Simulation;
		fire_control_.on_master_arm_changed();
		actions_.append(MissionActionKind::AbortCms);
		mode_.fire_control = FireControlMode::Navigation;
		break;
	case CommandId::SetMasterArmOn:
		mode_.master_arm = MasterArmMode::On;
		mode_.fire_control = FireControlMode::Navigation;
		break;
	case CommandId::SelectNavigationMode:
		mode_.fire_control = FireControlMode::Navigation;
		break;
	case CommandId::SelectDogfightMode:
	case CommandId::SelectMissileOverride:
		mode_.master_arm = MasterArmMode::On;
		mode_.fire_control =
		    command.id == CommandId::SelectDogfightMode ? FireControlMode::Dogfight : FireControlMode::MissileOverride;
		cycle_weapon = true;
		break;
	case CommandId::StartCmsForward:
	case CommandId::StartCmsAft:
	case CommandId::StartCmsLeft:
		if (mode_.master_arm == MasterArmMode::On)
		{
			const MissionActionKind action = command.id == CommandId::StartCmsForward
			                                     ? MissionActionKind::StartCmsForward
			                                 : command.id == CommandId::StartCmsAft ? MissionActionKind::StartCmsAft
			                                                                        : MissionActionKind::StartCmsLeft;
			actions_.append(action);
		}
		return;
	case CommandId::StartCmsRight:
		actions_.append(MissionActionKind::AbortCms);
		return;
	default: // CMS press remains reserved.
		return;
	}
	fire_control_.on_mode_selected(cycle_weapon);
}
}
