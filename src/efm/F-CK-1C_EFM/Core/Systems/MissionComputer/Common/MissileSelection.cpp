#include "MissileSelection.h"
#include "StoresQueries.h"
#include <algorithm>
#include <array>

namespace
{
using namespace Core;
using namespace Core::Systems::Mc;

std::string selection_type(const StoreStationState& station)
{
	if (station.weapon.registered)
		return station.weapon.selection_group;
	if (!station.weapon_type_identifier.empty())
		return station.weapon_type_identifier;
	return station.weapon_identifier.empty() ? "station:" + std::to_string(station.station_index)
	                                         : station.weapon_identifier;
}
}

namespace Core::Systems::Mc
{
bool is_sidewinder(const WeaponDescriptor& weapon) noexcept
{
	return weapon.family == "aim-9";
}

bool supports_radar_cue(const WeaponDescriptor& weapon) noexcept
{
	return weapon.radar_cue;
}

MissileCount count_selected_missile_group(const StoresInventorySnapshot& inventory, int station)
{
	MissileCount result;
	if (station < 0 || static_cast<std::size_t>(station) >= inventory.stations.size())
		return { 0, false };
	const auto group = selection_type(inventory.stations[station]);
	for (const auto& item : inventory.stations)
	{
		if (!item.observation.available || item.configuration == StoreConfiguration::Unknown ||
		    (item.count > 0 && (item.category == WeaponCategory::Unknown || item.role == WeaponRole::Unknown)))
			result.complete = false;
		if (item.observation.available && item.configuration == StoreConfiguration::Equipped && item.count > 0 &&
		    item.category == WeaponCategory::Missile && item.role == WeaponRole::AirToAir &&
		    selection_type(item) == group)
			result.count += item.count;
	}
	return result;
}

MissileSelection select_air_to_air_station(const StoresInventorySnapshot& inventory,
                                           std::optional<int> current_station,
                                           std::string_view current_type,
                                           MissileSelectionStep step)
{
	const auto candidates = find_missile_stations(inventory, { WeaponRole::AirToAir, {} });
	if (candidates.empty())
		return {};
	std::array<std::string, kWeaponStationCount> types;
	std::vector<std::string> rotary;
	for (int station : candidates)
	{
		types[station] = selection_type(inventory.stations[station]);
		if (std::find(rotary.begin(), rotary.end(), types[station]) == rotary.end())
			rotary.push_back(types[station]);
	}
	if (current_type.empty() && current_station &&
	    std::find(candidates.begin(), candidates.end(), *current_station) != candidates.end())
		current_type = types[*current_station];
	auto selected_type = std::find(rotary.begin(), rotary.end(), current_type);
	if (selected_type == rotary.end())
		selected_type = rotary.begin();
	else if (step == MissileSelectionStep::WeaponType)
	{
		++selected_type;
		if (selected_type == rotary.end())
			selected_type = rotary.begin();
	}
	std::vector<int> stations;
	for (int station : candidates)
		if (types[station] == *selected_type)
			stations.push_back(station);
	auto selected = std::find(stations.begin(), stations.end(), current_station.value_or(-1));
	if (selected == stations.end())
		selected = stations.begin();
	else if (step == MissileSelectionStep::Station)
	{
		++selected;
		if (selected == stations.end())
			selected = stations.begin();
	}
	return { *selected, *selected_type };
}
}
