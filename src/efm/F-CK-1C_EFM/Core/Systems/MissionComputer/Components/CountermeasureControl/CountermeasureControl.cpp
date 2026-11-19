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
	if (command.id == CommandId::StartCmsForward && Common::command_value_is_pressed(command.value_normalized))
		actions_.append(MissionActionKind::StartCmsForward);
}
}
