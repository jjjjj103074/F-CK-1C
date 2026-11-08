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
	const auto sequence = actions_.reserve();
	pending_.push_back({ sequence, command });
	return sequence;
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
		const auto sequence = enqueue(command);
		enqueue(SelectStoresForMode{ latest_mode_, false, true });
		return sequence;
	}
	return enqueue(command);
}
}
