#include "ExternalActionQueue.h"

#include <stdexcept>
#include <algorithm>

namespace Core::Systems::Mc
{
std::uint64_t ExternalActionQueue::append(MissionActionKind action)
{
	return append(MissionAction{ action });
}

std::uint64_t ExternalActionQueue::append(const MissionAction& action)
{
	if (action.kind == MissionActionKind::None)
		throw std::invalid_argument("MC action append requires an executable action.");
	if (last_sequence_ == kMaximumExactCockpitInteger)
		throw std::overflow_error("MC action sequence exhausted.");
	pending_.push_back(action);
	return ++last_sequence_;
}

void ExternalActionQueue::cancel_weapon_starts()
{
	for (auto& action : pending_)
		if (action.kind == MissionActionKind::SelectStation || is_weapon_start(action.kind))
			action = MissionAction{};
}

bool ExternalActionQueue::has_pending_selection() const noexcept
{
	return std::any_of(pending_.begin(),
	                   pending_.end(),
	                   [](const MissionAction& action)
	                   {
		                   return action.kind == MissionActionKind::SelectStation;
	                   });
}

void ExternalActionQueue::acknowledge(std::uint64_t sequence)
{
	if (sequence <= acknowledged_sequence_ || sequence > published_through_)
		return;
	while (acknowledged_sequence_ < sequence)
	{
		pending_.pop_front();
		++acknowledged_sequence_;
	}
}

bool ExternalActionQueue::acknowledged(std::uint64_t sequence) const noexcept
{
	return sequence != 0 && sequence <= acknowledged_sequence_;
}

MissionActionBatch ExternalActionQueue::batch()
{
	MissionActionBatch result;
	for (const auto& action : pending_)
	{
		if (result.count == result.actions.size())
			break;
		result.actions[result.count++] = action;
	}
	result.first_sequence = result.count == 0 ? 0 : acknowledged_sequence_ + 1;
	published_through_ = acknowledged_sequence_ + result.count;
	return result;
}
}
