#include "CountermeasureControl.h"

#include "Common/CommandValue.h"

namespace Core::Systems::Mc
{
CountermeasureControl::CountermeasureControl(ExternalActionOutbox& actions)
    : actions_(actions)
{
}

void CountermeasureControl::handle(const Command& command)
{
	if (!Common::command_value_is_pressed(command.value_normalized))
		return;
	switch (command.id)
	{
	case CommandId::StartCmsForward:
		actions_.append(MissionActionKind::StartCmsForward);
		break;
	case CommandId::StartCmsLeft:
		actions_.append(MissionActionKind::StartCmsProgram6);
		break;
	default:
		break;
	}
}
}
