#include "StoresCommandQueue.h"

#include <stdexcept>

namespace
{
using namespace Core;
using namespace Core::Systems::Mc;

bool is_start(MissionActionKind action)
{
	return action == MissionActionKind::GunOn || action == MissionActionKind::PickleOn ||
	       action == MissionActionKind::LaunchPermissionOverride;
}
}

namespace Core::Systems::Mc
{
StoresCommandQueue::StoresCommandQueue(ExternalActionQueue& actions)
    : actions_(actions)
{
}

std::uint64_t StoresCommandQueue::submit(const StoresCommand& command)
{
	if (const auto* weapon = std::get_if<StoresWeaponAction>(&command))
		if (!is_start(weapon->action) && weapon->action != MissionActionKind::GunOff &&
		    weapon->action != MissionActionKind::PickleOff)
			throw std::invalid_argument("Stores command requires a supported weapon operation.");
	const auto* mode = std::get_if<SelectStoresForMode>(&command);
	if (std::holds_alternative<CancelStoresCommands>(command) || (mode && mode->mode == OperatingMode::Navigation))
	{
		for (auto& entry : pending_)
		{
			const auto* weapon = std::get_if<StoresWeaponAction>(&entry.command);
			if (std::holds_alternative<SelectStoresForMode>(entry.command) || (weapon && is_start(weapon->action)))
				entry.canceled = true;
		}
	}
	const auto sequence = actions_.reserve();
	pending_.push_back({ sequence, command });
	return sequence;
}
}
