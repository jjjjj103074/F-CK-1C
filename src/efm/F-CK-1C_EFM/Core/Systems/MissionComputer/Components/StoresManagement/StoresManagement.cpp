#include "StoresManagement.h"

#include "Internal/ObservationProcessing.h"

#include <algorithm>

namespace Core::Systems::Mc
{
StoresManagement::StoresManagement(const MissionInputs& input,
                                   StoresData& stores,
                                   StoresEvents& events,
                                   StoresCommandQueue& commands,
                                   ExternalActionQueue& actions)
    : input_(input),
      stores_(stores),
      events_(events),
      commands_(commands),
      actions_(actions),
      selection_(stores.inventory, stores.selection),
      gun_execution_(stores.weapon_use.gun, commands),
      release_execution_(stores.weapon_use.release, commands)
{
}

MissionAction StoresManagement::selection_action() const
{
	return { MissionActionKind::SelectStation,
		     stores_.selection.requested_station,
		     stores_.selection.request_revision };
}

void StoresManagement::prepare(std::chrono::nanoseconds time)
{
	accept_commands();
	time = (std::max)(time, input_.observed_at);
	const auto change = Internal::update_inventory(stores_.inventory, input_.cockpit.weapon_stations, time);
	inventory_change_.revision = change.revision;
	inventory_change_.changed_stations |= change.changed_stations;
	inventory_change_.rearmed = inventory_change_.rearmed || change.rearmed;
	gun_changed_ =
	    Internal::update_gun_ammunition(stores_.gun_ammunition, input_.cockpit.gun_ammunition, time) || gun_changed_;
	const auto previous_request = stores_.selection.request_revision;
	const bool changed = pending_commands_.empty() ? selection_.update(input_.cockpit.weapon_stations)
	                                               : selection_.acknowledge(input_.cockpit.weapon_stations);
	selection_changed_ = changed || selection_changed_;
	if (stores_.selection.request_revision != previous_request)
		actions_.append(selection_action());
}

void StoresManagement::dispatch()
{
	// 接受通知期間送入的要求，保存停止與取消的處理順序。
	accept_commands();
	if (dispatching_)
		return;
	dispatching_ = true;
	try
	{
		drain_commands();
		notify_changes();
	}
	catch (...)
	{
		dispatching_ = false;
		throw;
	}
	dispatching_ = false;
}

void StoresManagement::cancel_pending_commands()
{
	cancel_pending_weapon_starts();
	for (auto& entry : pending_commands_)
		if (std::holds_alternative<SelectStoresForMode>(entry.command))
			entry.canceled = true;
}

void StoresManagement::cancel_pending_weapon_starts()
{
	gun_execution_.cancel_pending();
	release_execution_.cancel_pending();
	for (auto& entry : pending_commands_)
	{
		const auto* weapon = std::get_if<StoresWeaponAction>(&entry.command);
		if (weapon && is_weapon_start(weapon->action))
			entry.canceled = true;
	}
}

void StoresManagement::cancel_dispatched_commands()
{
	actions_.cancel_stores_commands();
	selection_changed_ = selection_.cancel_pending() || selection_changed_;
}

bool StoresManagement::has_pending_mode() const
{
	return std::any_of(pending_commands_.begin(),
	                   pending_commands_.end(),
	                   [](const PendingCommand& entry)
	                   {
		                   const auto* mode = std::get_if<SelectStoresForMode>(&entry.command);
		                   return !entry.canceled && mode && mode->mode != OperatingMode::Navigation;
	                   });
}

void StoresManagement::accept_commands()
{
	while (const auto entry = commands_.take_next())
	{
		if (accept_weapon_use(*entry))
			continue;
		if (std::holds_alternative<CancelWeaponStarts>(entry->command))
		{
			cancel_pending_weapon_starts();
			actions_.cancel_weapon_starts();
			commands_.resolve(entry->ticket, 0);
			continue;
		}
		const auto* mode = std::get_if<SelectStoresForMode>(&entry->command);
		if (mode)
			latest_mode_ = mode->mode;
		if (std::holds_alternative<CancelStoresCommands>(entry->command) ||
		    (mode && mode->mode == OperatingMode::Navigation))
			cancel_pending_commands();
		schedule_command({ entry->ticket, entry->command });
	}
}

void StoresManagement::schedule_command(PendingCommand entry)
{
	const auto* weapon = std::get_if<StoresWeaponAction>(&entry.command);
	if (weapon && is_weapon_stop(weapon->action) && (has_pending_mode() || actions_.has_pending_selection()))
	{
		// 撤回等待選站的前綴，先停止，再依最新模式重新確認掛點。
		cancel_pending_commands();
		pending_commands_.push_back({ std::nullopt, CancelStoresCommands{} });
		pending_commands_.push_back(entry);
		pending_commands_.push_back({ std::nullopt, SelectStoresForMode{ latest_mode_, false, true } });
	}
	else
		pending_commands_.push_back(entry);
}

void StoresManagement::drain_commands()
{
	const bool previously_awaiting = stores_.selection.awaiting_inventory;
	stores_.selection.awaiting_inventory = false;
	while (!pending_commands_.empty())
	{
		const auto& entry = pending_commands_.front();
		MissionAction action;
		if (!entry.canceled)
		{
			if (const auto* mode = std::get_if<SelectStoresForMode>(&entry.command))
			{
				if (mode->mode != OperatingMode::Navigation && !stores_.inventory.observation.available)
				{
					stores_.selection.awaiting_inventory = true;
					break;
				}
				if (mode->mode == OperatingMode::Navigation)
					cancel_dispatched_commands();
				const auto previous_request = stores_.selection.request_revision;
				selection_.select_mode(mode->mode, mode->cycle_weapon, mode->reassert_selection);
				selection_changed_ = selection_.update(input_.cockpit.weapon_stations) || selection_changed_;
				if (stores_.selection.request_revision != previous_request)
					action = selection_action();
			}
			else if (const auto* weapon = std::get_if<StoresWeaponAction>(&entry.command))
				action.kind = weapon->action;
			else
				cancel_dispatched_commands();
		}
		const auto sequence = action.kind == MissionActionKind::None ? 0 : actions_.append(action);
		record_weapon_dispatch(entry, action.kind, sequence);
		if (entry.ticket)
			commands_.resolve(*entry.ticket, sequence);
		pending_commands_.pop_front();
	}
	selection_changed_ = selection_changed_ || previously_awaiting != stores_.selection.awaiting_inventory;
}

void StoresManagement::step(std::chrono::nanoseconds time, double)
{
	prepare(time);
	advance_weapon_use((std::max)(time, input_.observed_at));
	dispatch();
}

void StoresManagement::notify_changes()
{
	// 完成資料寫入後，依序通知庫存、彈量與選站變化。
	if (inventory_change_.changed_stations.any() || inventory_change_.rearmed)
		events_.inventory_changed(inventory_change_);
	if (gun_changed_)
		events_.gun_ammunition_changed(GunAmmunitionChanged{ stores_.gun_ammunition.revision });
	if (selection_changed_)
		events_.selection_changed(SelectionChanged{ stores_.selection.request_revision,
		                                            stores_.selection.acknowledged_revision,
		                                            stores_.selection.awaiting_inventory,
		                                            stores_.selection.canceled_through_revision });
	inventory_change_ = {};
	gun_changed_ = false;
	selection_changed_ = false;
}
}
