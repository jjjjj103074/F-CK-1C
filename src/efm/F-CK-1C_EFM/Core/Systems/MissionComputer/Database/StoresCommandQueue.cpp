#include "StoresCommandQueue.h"

#include <stdexcept>

namespace Core::Systems::Mc
{
StoresCommandQueue::StoresCommandQueue(ExternalActionOutbox& actions)
    : actions_(actions)
{
}

std::uint64_t StoresCommandQueue::enqueue(const StoresCommand& command)
{
	if (last_ticket_ == kMaximumExactCockpitInteger)
		throw std::overflow_error("Stores command ticket exhausted.");
	receipts_.push_back({});
	incoming_.push_back({ ++last_ticket_, command });
	return last_ticket_;
}

void StoresCommandQueue::resolve(std::uint64_t ticket, std::uint64_t action_request_sequence)
{
	auto& receipt = receipts_.at(static_cast<std::size_t>(ticket - retired_through_ - 1));
	receipt = { true, action_request_sequence };
	retire_completed();
}

std::optional<QueuedStoresCommand> StoresCommandQueue::take_next()
{
	retire_completed();
	if (incoming_.empty())
		return std::nullopt;
	const auto request = incoming_.front();
	incoming_.pop_front();
	return request;
}

bool StoresCommandQueue::acknowledged(std::uint64_t ticket) const noexcept
{
	if (ticket == 0 || ticket > last_ticket_)
		return false;
	if (ticket <= retired_through_)
		return true;
	const auto& receipt = receipts_[static_cast<std::size_t>(ticket - retired_through_ - 1)];
	return receipt.resolved &&
	       (receipt.action_request_sequence == 0 || actions_.acknowledged(receipt.action_request_sequence));
}

void StoresCommandQueue::retire_completed()
{
	while (!receipts_.empty() && acknowledged(retired_through_ + 1))
	{
		receipts_.pop_front();
		++retired_through_;
	}
}

std::uint64_t StoresCommandQueue::submit(const StoresCommand& command)
{
	const auto* weapon = std::get_if<StoresWeaponAction>(&command);
	if (weapon && !is_weapon_start(weapon->action) && !is_weapon_stop(weapon->action))
		throw std::invalid_argument("Stores command requires a supported weapon operation.");
	const auto ticket = enqueue(command);
	commands_available();
	return ticket;
}
}
