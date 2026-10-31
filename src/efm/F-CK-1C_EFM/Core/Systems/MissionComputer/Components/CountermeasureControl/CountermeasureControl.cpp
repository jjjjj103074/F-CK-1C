#include "CountermeasureControl.h"

#include "Common/CommandValue.h"

namespace Core::Systems::Mc
{
CountermeasureControl::CountermeasureControl(const ModeData& mode, ExternalActionQueue& actions)
    : mode_(mode),
      actions_(actions)
{
}

void CountermeasureControl::on_master_arm_selected(const MasterArmSelected& event)
{
	// 依本次事件的 OFF/SIM 狀態發出中止要求。
	if (event.selected.master_arm != MasterArmMode::On)
		actions_.append(MissionActionKind::AbortCms);
}

void CountermeasureControl::handle(const Command& command)
{
	if (!Common::command_value_is_pressed(command.value_normalized))
		return;
	switch (command.id)
	{
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
		break;
	case CommandId::StartCmsRight:
		// 右向操作發出中止要求。
		actions_.append(MissionActionKind::AbortCms);
		break;
	default:
		break;
	}
}
}
