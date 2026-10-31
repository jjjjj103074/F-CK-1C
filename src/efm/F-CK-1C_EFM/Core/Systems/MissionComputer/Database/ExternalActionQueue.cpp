#include "ExternalActionQueue.h"

#include <algorithm>
#include <stdexcept>

namespace Core::Systems::Mc
{
std::uint64_t ExternalActionQueue::append(MissionActionKind action)
{
	if (action == MissionActionKind::None || last_sequence_ == kMaximumExactCockpitInteger)
	{
		throw std::overflow_error("Invalid or exhausted MC action sequence.");
	}
	pending_.push_back(action);
	return ++last_sequence_;
}

void ExternalActionQueue::acknowledge(std::uint64_t sequence)
{
	// 依已發布視窗確認序號，並移除已執行動作。
	if (sequence <= acknowledged_sequence_ || sequence > published_through_)
	{
		return;
	}
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
	result.count = (std::min)(pending_.size(), result.actions.size());
	result.first_sequence = result.count == 0 ? 0 : acknowledged_sequence_ + 1;
	std::copy_n(pending_.begin(), result.count, result.actions.begin());
	published_through_ = acknowledged_sequence_ + result.count;
	return result;
}
}
