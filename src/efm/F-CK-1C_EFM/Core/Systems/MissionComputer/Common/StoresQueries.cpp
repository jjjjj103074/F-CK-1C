#include "StoresQueries.h"

namespace
{
using namespace Core;
using namespace Core::Systems::Mc;

bool matches(const StoreStationState& station, const MissileQuery& query)
{
	return station.observation.available && station.configuration == StoreConfiguration::Equipped &&
	       station.count > 0 && station.category == WeaponCategory::Missile &&
	       (!query.role || station.role == *query.role) &&
	       (!query.weapon_identifier || station.weapon_identifier == *query.weapon_identifier);
}
}

namespace Core::Systems::Mc
{
std::vector<int> find_missile_stations(const StoresInventorySnapshot& inventory, const MissileQuery& query)
{
	std::vector<int> result;
	for (const auto& station : inventory.stations)
		if (matches(station, query))
			result.push_back(station.station_index);
	return result;
}

MissileCount count_missiles(const StoresInventorySnapshot& inventory, const MissileQuery& query)
{
	MissileCount result;
	for (const auto& station : inventory.stations)
	{
		if (!station.observation.available || station.configuration == StoreConfiguration::Unknown)
			result.complete = false;
		if (station.configuration == StoreConfiguration::Equipped && station.count > 0 &&
		    (station.category == WeaponCategory::Unknown ||
		     (station.category == WeaponCategory::Missile && query.role && *query.role != WeaponRole::Unknown &&
		      station.role == WeaponRole::Unknown)))
			result.complete = false;
		if (matches(station, query))
			result.count += station.count;
	}
	return result;
}
}
