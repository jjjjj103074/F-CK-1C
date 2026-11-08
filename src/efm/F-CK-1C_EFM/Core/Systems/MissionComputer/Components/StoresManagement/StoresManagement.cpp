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
      selection_(stores.inventory, stores.selection)
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
	commands_.retire_completed();
	time = (std::max)(time, input_.observed_at);
	const auto change = Internal::update_inventory(stores_.inventory, input_.cockpit.weapon_stations, time);
	inventory_change_.revision = change.revision;
	inventory_change_.changed_stations |= change.changed_stations;
	inventory_change_.rearmed = inventory_change_.rearmed || change.rearmed;
	gun_changed_ =
	    Internal::update_gun_ammunition(stores_.gun_ammunition, input_.cockpit.gun_ammunition, time) || gun_changed_;
	const auto previous_request = stores_.selection.request_revision;
	const bool changed = commands_.pending_.empty() ? selection_.update(input_.cockpit.weapon_stations)
	                                                : selection_.acknowledge(input_.cockpit.weapon_stations);
	selection_changed_ = changed || selection_changed_;
	if (stores_.selection.request_revision != previous_request)
		actions_.append(selection_action());
}

void StoresManagement::dispatch()
{
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

void StoresManagement::drain_commands()
{
	const bool previously_awaiting = stores_.selection.awaiting_inventory;
	stores_.selection.awaiting_inventory = false;
	while (!commands_.pending_.empty())
	{
		const auto& entry = commands_.pending_.front();
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
					actions_.cancel_weapon_starts();
				const auto previous_request = stores_.selection.request_revision;
				selection_.select_mode(mode->mode, mode->cycle_weapon, mode->reassert_selection);
				selection_changed_ = selection_.update(input_.cockpit.weapon_stations) || selection_changed_;
				if (stores_.selection.request_revision != previous_request)
					action = selection_action();
			}
			else if (const auto* weapon = std::get_if<StoresWeaponAction>(&entry.command))
				action.kind = weapon->action;
			else
				actions_.cancel_weapon_starts();
		}
		const auto sequence = action.kind == MissionActionKind::None ? 0 : actions_.append(action);
		commands_.resolve(entry.ticket, sequence);
		commands_.pending_.pop_front();
	}
	commands_.retire_completed();
	selection_changed_ = selection_changed_ || previously_awaiting != stores_.selection.awaiting_inventory;
}

void StoresManagement::step(std::chrono::nanoseconds time, double)
{
	prepare(time);
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
		                                            stores_.selection.awaiting_inventory });
	inventory_change_ = {};
	gun_changed_ = false;
	selection_changed_ = false;
}
}
