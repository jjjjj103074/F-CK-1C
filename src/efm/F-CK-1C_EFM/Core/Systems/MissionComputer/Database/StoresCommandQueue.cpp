#include "StoresCommandQueue.h"

#include <algorithm>
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
	pending_.push_back({ ++last_ticket_, command });
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

void StoresCommandQueue::cancel_pending()
{
	for (auto& entry : pending_)
	{
		const auto* weapon = std::get_if<StoresWeaponAction>(&entry.command);
		if (std::holds_alternative<SelectStoresForMode>(entry.command) || (weapon && is_weapon_start(weapon->action)))
			entry.canceled = true;
	}
}

bool StoresCommandQueue::has_pending_mode() const
{
	return std::any_of(pending_.begin(),
	                   pending_.end(),
	                   [](const Entry& entry)
	                   {
		                   const auto* mode = std::get_if<SelectStoresForMode>(&entry.command);
		                   return !entry.canceled && mode && mode->mode != OperatingMode::Navigation;
	                   });
}

std::uint64_t StoresCommandQueue::submit(const StoresCommand& command)
{
	const auto* weapon = std::get_if<StoresWeaponAction>(&command);
	if (weapon && !is_weapon_start(weapon->action) && !is_weapon_stop(weapon->action))
		throw std::invalid_argument("Stores command requires a supported weapon operation.");
	const auto* mode = std::get_if<SelectStoresForMode>(&command);
	if (mode)
		latest_mode_ = mode->mode;
	if (std::holds_alternative<CancelStoresCommands>(command) || (mode && mode->mode == OperatingMode::Navigation))
		cancel_pending();
	if (weapon && is_weapon_stop(weapon->action) && (has_pending_mode() || actions_.has_pending_selection()))
	{
		// 撤回等待選站的前綴，先停止，再以最新模式重建選站。
		cancel_pending();
		enqueue(CancelStoresCommands{});
		const auto ticket = enqueue(command);
		enqueue(SelectStoresForMode{ latest_mode_, false, true });
		commands_available();
		return ticket;
	}
	const auto ticket = enqueue(command);
	commands_available();
	return ticket;
}
}
