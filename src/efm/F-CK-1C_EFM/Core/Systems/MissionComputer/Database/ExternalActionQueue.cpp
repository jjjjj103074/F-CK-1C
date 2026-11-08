#include "ExternalActionQueue.h"

#include <stdexcept>

namespace Core::Systems::Mc
{
std::uint64_t ExternalActionQueue::reserve()
{
	if (last_sequence_ == kMaximumExactCockpitInteger)
		throw std::overflow_error("MC action sequence exhausted.");
	pending_.push_back(std::nullopt);
	return ++last_sequence_;
}

void ExternalActionQueue::resolve(std::uint64_t sequence, const MissionAction& action)
{
	if (sequence <= acknowledged_sequence_ || sequence > last_sequence_)
		throw std::out_of_range("MC action reservation is outside the pending range.");
	auto& entry = pending_.at(static_cast<std::size_t>(sequence - acknowledged_sequence_ - 1));
	if (entry)
		throw std::logic_error("MC action reservation is already resolved.");
	entry = action;
}

std::uint64_t ExternalActionQueue::append(MissionActionKind action)
{
	return append(MissionAction{ action });
}

std::uint64_t ExternalActionQueue::append(const MissionAction& action)
{
	if (action.kind == MissionActionKind::None)
		throw std::invalid_argument("MC action append requires an executable action.");
	const auto sequence = reserve();
	resolve(sequence, action);
	return sequence;
}

void ExternalActionQueue::cancel_weapon_starts()
{
	for (auto& action : pending_)
		if (action && (action->kind == MissionActionKind::SelectStation || action->kind == MissionActionKind::GunOn ||
		               action->kind == MissionActionKind::PickleOn ||
		               action->kind == MissionActionKind::LaunchPermissionOverride))
			action = MissionAction{};
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
		if (!action || result.count == result.actions.size())
			break;
		result.actions[result.count++] = *action;
	}
	result.first_sequence = result.count == 0 ? 0 : acknowledged_sequence_ + 1;
	published_through_ = acknowledged_sequence_ + result.count;
	return result;
}
}
