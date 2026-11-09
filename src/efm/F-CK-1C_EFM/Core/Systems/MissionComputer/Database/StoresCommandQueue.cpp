#include "StoresCommandQueue.h"

#include <stdexcept>

namespace Core::Systems::Mc
{
StoresCommandQueue::StoresCommandQueue(ExternalActionQueue& actions)
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

void StoresCommandQueue::resolve(std::uint64_t ticket, std::uint64_t external_sequence)
{
	auto& receipt = receipts_.at(static_cast<std::size_t>(ticket - retired_through_ - 1));
	receipt = { true, external_sequence };
}

bool StoresCommandQueue::acknowledged(std::uint64_t ticket) const noexcept
{
	if (ticket == 0 || ticket > last_ticket_)
		return false;
	if (ticket <= retired_through_)
		return true;
	const auto& receipt = receipts_[static_cast<std::size_t>(ticket - retired_through_ - 1)];
	return receipt.resolved && (receipt.external_sequence == 0 || actions_.acknowledged(receipt.external_sequence));
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
