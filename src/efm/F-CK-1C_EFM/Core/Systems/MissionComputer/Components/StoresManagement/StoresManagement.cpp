#include "StoresManagement.h"

#include <algorithm>
#include <stdexcept>

namespace
{
constexpr double kFallbackGunRoundsPerSecond = 100.0;
}

namespace Core::Systems::Mc
{
StoresManagement::StoresManagement(const MissionInputs& input,
                                   const ModeData& mode,
                                   const WeaponControlData& weapon,
                                   StoresData& stores)
    : input_(input),
      mode_(mode),
      weapon_(weapon),
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

void StoresManagement::update_gun_quantity(std::chrono::nanoseconds time)
{
	time = (std::max)(time, input_.observed_at);
	const auto& adapter = input_.cockpit.mission_adapter;
	if (adapter.gun_count_available)
		stores_.gun_count = adapter.gun_count;
	else if (weapon_.gun_requested && gun_quantity_sequence_ != 0 &&
	         gun_quantity_sequence_ == weapon_.gun_start_sequence)
	{
		const double elapsed_s = std::chrono::duration<double>(time - gun_quantity_time_).count();
		stores_.gun_count = (std::max)(0.0, stores_.gun_count - kFallbackGunRoundsPerSecond * elapsed_s);
	}
	// New commands belong to the current input time, including during scheduler catch-up.
	gun_quantity_time_ = time;
	gun_quantity_sequence_ = weapon_.gun_requested ? weapon_.gun_start_sequence : 0;
}

void StoresManagement::step(std::chrono::nanoseconds time, double)
{
	update_gun_quantity(time);
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
