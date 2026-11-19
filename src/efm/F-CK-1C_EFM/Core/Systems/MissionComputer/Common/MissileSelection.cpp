#include "MissileSelection.h"
#include "StoresQueries.h"
#include <algorithm>
#include <array>

namespace
{
using namespace Core;
using namespace Core::Systems::Mc;

bool sidewinder_lm(std::string_view identifier)
{
	// SMS 的 A-9LM 選項合併 L／M；其他型別保留來源識別。
	constexpr std::string_view identifiers[] = { "{AIM-9L}",
		                                         "{AIM-9M}",
		                                         "{6CEB49FC-DED8-4DED-B053-E1F033FF72D3}",
		                                         "{AIM-9L-ON-ADAPTER}",
		                                         "{AIM-9M-ON-ADAPTER}",
		                                         "LAU-127_AIM-9L",
		                                         "LAU-127_AIM-9M" };
	return std::find(std::begin(identifiers), std::end(identifiers), identifier) != std::end(identifiers);
}

std::string selection_type(const StoresInventorySnapshot& inventory, const StoreStationState& station)
{
	if (sidewinder_lm(station.weapon_identifier))
		return "A-9LM";
	if (!station.weapon_type_identifier.empty())
	{
		for (const auto& other : inventory.stations)
			if (other.observation.available && other.configuration == StoreConfiguration::Equipped &&
			    other.category == WeaponCategory::Missile && other.role == WeaponRole::AirToAir &&
			    other.weapon_type_identifier == station.weapon_type_identifier &&
			    sidewinder_lm(other.weapon_identifier))
				return "A-9LM";
		return station.weapon_type_identifier;
	}
	return station.weapon_identifier.empty() ? "station:" + std::to_string(station.station_index)
	                                         : station.weapon_identifier;
}
}

namespace Core::Systems::Mc
{
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
		types[station] = selection_type(inventory, inventory.stations[station]);
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
