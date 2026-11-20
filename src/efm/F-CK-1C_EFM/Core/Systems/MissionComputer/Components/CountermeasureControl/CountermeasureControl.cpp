#include "CountermeasureControl.h"
#include "Common/CommandValue.h"
#include "../../../../Contracts/MissionActions.h"
#include <algorithm>
#include <stdexcept>
namespace Core::Systems::Mc
{
void CountermeasureControl::handle(const Command& command)
{
	if (!Common::command_value_is_pressed(command.value_normalized))
		return;
	CountermeasureProgram program;
	switch (command.id)
	{
	case CommandId::StartCmsForward:
		program = CountermeasureProgram::Forward;
		break;
	case CommandId::StartCmsLeft:
		program = CountermeasureProgram::Program6;
		break;
	default:
		return;
	}
	if (last_sequence_ == kMaximumExactCockpitInteger)
		throw std::overflow_error("Countermeasure program sequence exhausted.");
	pending_.items.push_back({ ++last_sequence_, program });
}
void CountermeasureControl::acknowledge(std::uint64_t sequence)
{
	if (sequence > last_sequence_)
		return;
	auto& items = pending_.items;
	items.erase(std::remove_if(items.begin(),
	                           items.end(),
	                           [sequence](const auto& request)
	                           {
		                           return request.sequence <= sequence;
	                           }),
	            items.end());
}
}
