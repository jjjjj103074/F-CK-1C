#include "StoresManagement.h"

namespace Core::Systems::Mc
{
Internal::WeaponExecution& StoresManagement::weapon_execution(WeaponUseKind kind)
{
	return kind == WeaponUseKind::Gun ? gun_execution_ : release_execution_;
}

bool StoresManagement::accept_weapon_use(const QueuedStoresCommand& entry)
{
	if (const auto* begin = std::get_if<BeginWeaponUse>(&entry.command))
	{
		if (!weapon_execution(begin->kind).begin(entry.ticket, input_.cockpit.mission_adapter.available))
		{
			commands_.resolve(entry.ticket, 0);
			return true;
		}
		if (begin->kind == WeaponUseKind::Release && begin->launch_permission == LaunchPermission::Override)
			schedule_command({ std::nullopt, StoresWeaponAction{ MissionActionKind::LaunchPermissionOverride } });
		const auto action = begin->kind == WeaponUseKind::Gun ? MissionActionKind::GunOn : MissionActionKind::PickleOn;
		schedule_command({ entry.ticket, StoresWeaponAction{ action } });
		return true;
	}
	if (const auto* end = std::get_if<EndWeaponUse>(&entry.command))
	{
		weapon_execution(end->use.kind).request_end(end->use.request_id);
		commands_.resolve(entry.ticket, 0);
		return true;
	}
	if (!std::holds_alternative<AbortWeaponUse>(entry.command))
		return false;

	cancel_pending_weapon_starts();
	actions_.cancel_weapon_starts();
	const bool stop_gun = gun_execution_.abort();
	const bool stop_release = release_execution_.abort();
	if (stop_gun)
		schedule_command({ std::nullopt, StoresWeaponAction{ MissionActionKind::GunOff } });
	if (stop_release)
		schedule_command({ std::nullopt, StoresWeaponAction{ MissionActionKind::PickleOff } });
	commands_.resolve(entry.ticket, 0);
	return true;
}

void StoresManagement::advance_weapon_use(std::chrono::nanoseconds time)
{
	if (gun_execution_.end_due(time))
		schedule_command({ std::nullopt,
		                   StoresWeaponAction{ MissionActionKind::GunOff },
		                   false,
		                   WeaponUseReference{ WeaponUseKind::Gun, stores_.weapon_use.gun.request_id } });
	if (release_execution_.end_due(time))
		schedule_command({ std::nullopt,
		                   StoresWeaponAction{ MissionActionKind::PickleOff },
		                   false,
		                   WeaponUseReference{ WeaponUseKind::Release, stores_.weapon_use.release.request_id } });
}

void StoresManagement::record_weapon_dispatch(const PendingCommand& entry,
                                              MissionActionKind action,
                                              std::uint64_t sequence)
{
	if (!entry.use || sequence == 0)
		return;
	auto& execution = weapon_execution(entry.use->kind);
	if (is_weapon_stop(action))
		execution.stop_scheduled(entry.use->request_id);
}
}
