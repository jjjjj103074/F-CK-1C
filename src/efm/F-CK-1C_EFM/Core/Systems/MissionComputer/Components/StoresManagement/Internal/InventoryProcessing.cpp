#include "InventoryProcessing.h"

#include <limits>
#include <stdexcept>

namespace Core::Systems::Mc::Internal
{
bool update_station(StoreStationState& station,
                    const WeaponStationSample& sample,
                    const ObservationStatus& observation,
                    std::chrono::nanoseconds time,
                    bool rearmed)
{
	bool changed = station.observation.available != observation.available ||
	               station.observation.invalid_reason != observation.invalid_reason;
	station.observation = observation;
	if (!observation.available)
		return changed;

	const bool identity_changed = station.present != sample.present ||
	                              station.weapon_identifier != sample.weapon_identifier ||
	                              station.weapon_type_identifier != sample.weapon_type_identifier ||
	                              station.category != sample.category || station.role != sample.role;
	const bool load_changed = (sample.present && !station.last_observed_at) ||
	                          (station.last_observed_at && identity_changed) ||
	                          (rearmed && (station.present || sample.present));
	if (load_changed)
	{
		if (station.load_revision == (std::numeric_limits<std::uint64_t>::max)())
			throw std::overflow_error("MC station load revision exhausted.");
		++station.load_revision;
	}
	changed = changed || identity_changed || station.count != sample.count ||
	          station.station_name != sample.station_name || load_changed;
	station.station_name = sample.station_name;
	station.present = sample.present;
	station.count = sample.count;
	station.category = sample.category;
	station.role = sample.role;
	station.weapon_identifier = sample.weapon_identifier;
	station.weapon_type_identifier = sample.weapon_type_identifier;
	station.last_observed_at = time;
	return changed;
}
}
