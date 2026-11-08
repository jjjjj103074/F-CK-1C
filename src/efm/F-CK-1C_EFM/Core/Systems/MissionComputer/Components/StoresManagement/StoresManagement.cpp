#include "StoresManagement.h"

#include "Internal/ObservationProcessing.h"

#include <algorithm>

namespace Core::Systems::Mc
{
StoresManagement::StoresManagement(const MissionInputs& input, StoresData& stores, StoresEvents& events)
    : input_(input),
      stores_(stores),
      events_(events),
      selection_(stores.inventory, stores.selection)
{
}

void StoresManagement::on_operating_mode_selected(const OperatingModeSelected& event)
{
	selection_.select_mode(event.selected.operating_mode, event.cycle_weapon);
}

void StoresManagement::step(std::chrono::nanoseconds time, double)
{
	time = (std::max)(time, input_.observed_at);
	const auto inventory_change = Internal::update_inventory(stores_.inventory, input_.cockpit.weapon_stations, time);
	const bool gun_changed =
	    Internal::update_gun_ammunition(stores_.gun_ammunition, input_.cockpit.gun_ammunition, time);
	const bool selection_changed = selection_.update(input_.cockpit.weapon_stations);

	// 所有資料分區完成寫入後，依序通知庫存、彈量與選站變化。
	if (inventory_change.changed_stations.any() || inventory_change.rearmed)
		events_.inventory_changed(inventory_change);
	if (gun_changed)
		events_.gun_ammunition_changed(GunAmmunitionChanged{ stores_.gun_ammunition.revision });
	if (selection_changed)
		events_.selection_changed(
		    SelectionChanged{ stores_.selection.request_revision, stores_.selection.acknowledged_revision });
}
}
