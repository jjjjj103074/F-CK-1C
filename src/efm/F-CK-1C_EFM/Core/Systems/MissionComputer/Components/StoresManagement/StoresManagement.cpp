#include "StoresManagement.h"

#include <algorithm>
#include <stdexcept>

namespace Core::Systems::Mc
{
StoresManagement::StoresManagement(const MissionInputs& input, const ModeData& mode, StoresData& stores)
    : input_(input),
      mode_(mode),
      stores_(stores)
{
}

bool StoresManagement::is_air_to_air_missile(const WeaponStationSample& sample)
{
	// Preserve the prototype Lua classification until a weapon definition database replaces it.
	return sample.present && sample.count > 0 &&
	       ((sample.category == WeaponCategory::Missile && sample.role == WeaponRole::AirToAir) ||
	        sample.weapon_identifier.find("AIM-9") != std::string::npos ||
	        sample.weapon_identifier.find("AIM_9") != std::string::npos ||
	        sample.weapon_identifier.find("CATM-9") != std::string::npos);
}

void StoresManagement::request_station(std::optional<int> station, bool force)
{
	if (!force && stores_.requested_station == station)
		return;
	if (stores_.selection_request_revision == kMaximumExactCockpitInteger)
		throw std::overflow_error("MC station request revision exhausted its exact numeric range.");
	stores_.requested_station = station;
	++stores_.selection_request_revision;
}

void StoresManagement::step(std::chrono::nanoseconds, double)
{
	const auto& inventory = input_.cockpit.weapon_stations;
	const auto& adapter = input_.cockpit.mission_adapter;
	std::optional<int> first_loaded;
	if (inventory.status.available)
	{
		int count = 0;
		for (std::size_t i = 0; i < inventory.stations.size(); ++i)
		{
			if (is_air_to_air_missile(inventory.stations[i]))
			{
				count += inventory.stations[i].count;
				if (!first_loaded)
					first_loaded = static_cast<int>(i);
			}
		}
		stores_.missile_count = count;
		stores_.count_source = StoresCountSource::StationInventory;
		if (inventory.selection_ack_revision == stores_.selection_request_revision &&
		    inventory.selected_station == stores_.requested_station)
			stores_.selected_station = inventory.selected_station;
	}
	else if (adapter.missile_count_available)
	{
		stores_.missile_count = (std::max)(0, adapter.missile_count);
		stores_.count_source = StoresCountSource::BaseDataApi;
	}
	else
		stores_.count_source = StoresCountSource::Estimate;

	if (!mode_.air_to_air())
	{
		request_station(std::nullopt, false);
		selection_mode_ = mode_.fire_control;
	}
	else if (inventory.status.available)
	{
		request_station(first_loaded,
		                selection_mode_ != mode_.fire_control || rearm_revision_ != inventory.rearm_revision);
		selection_mode_ = mode_.fire_control;
		rearm_revision_ = inventory.rearm_revision;
	}
}

void StoresManagement::record_missile_release()
{
	if (stores_.count_source == StoresCountSource::Estimate)
		stores_.missile_count = (std::max)(0, stores_.missile_count - 1);
}
}
