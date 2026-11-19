#include "ObservationProcessing.h"

#include <limits>
#include <stdexcept>

namespace
{
using namespace Core;
constexpr auto kMaximumObservationAge = std::chrono::milliseconds(500);

ObservationStatus effective_status(ObservationStatus status,
                                   const std::optional<std::chrono::nanoseconds>& last_observed_at,
                                   std::chrono::nanoseconds time)
{
	if (status.available && status.revision == 0)
		return { false, 0, ObservationInvalidReason::InvalidRevision };
	if (status.available && last_observed_at && time - *last_observed_at > kMaximumObservationAge)
		return { false, status.revision, ObservationInvalidReason::Stale };
	return status;
}

bool update_station(StoreStationState& station,
                    const WeaponStationSample& sample,
                    const ObservationStatus& observation,
                    std::chrono::nanoseconds time,
                    bool rearmed,
                    bool new_batch)
{
	bool changed = station.observation.available != observation.available ||
	               station.observation.invalid_reason != observation.invalid_reason;
	station.observation = observation;
	if (!observation.available || !new_batch)
		return changed;

	const bool identity_changed = station.configuration != sample.configuration ||
	                              station.weapon_identifier != sample.weapon_identifier ||
	                              station.weapon_type_identifier != sample.weapon_type_identifier ||
	                              station.category != sample.category || station.role != sample.role;
	const bool load_changed = (sample.configuration == StoreConfiguration::Equipped && !station.last_observed_at) ||
	                          (station.last_observed_at && identity_changed) ||
	                          (rearmed && (station.configuration == StoreConfiguration::Equipped ||
	                                       sample.configuration == StoreConfiguration::Equipped));
	if (load_changed)
	{
		if (station.load_revision == (std::numeric_limits<std::uint64_t>::max)())
			throw std::overflow_error("MC station load revision exhausted.");
		++station.load_revision;
	}
	changed = changed || identity_changed || station.count != sample.count ||
	          station.station_name != sample.station_name || load_changed;
	station.station_name = sample.station_name;
	station.configuration = sample.configuration;
	station.count = sample.count;
	station.category = sample.category;
	station.role = sample.role;
	station.weapon_identifier = sample.weapon_identifier;
	station.weapon_type_identifier = sample.weapon_type_identifier;
	station.last_observed_at = time;
	return changed;
}
}

namespace Core::Systems::Mc::Internal
{
InventoryChanged update_inventory(StoresInventorySnapshot& inventory,
                                  const WeaponStationObservation& observation,
                                  std::chrono::nanoseconds time)
{
	const bool new_batch =
	    observation.status.available && observation.status.revision != 0 &&
	    (!inventory.last_observed_at || observation.status.revision != inventory.last_observed_revision);
	const bool rearmed = new_batch && observation.rearm_revision != inventory.rearm_revision;
	if (new_batch)
	{
		inventory.last_observed_at = time;
		inventory.last_observed_revision = observation.status.revision;
		inventory.rearm_revision = observation.rearm_revision;
	}
	inventory.observation = effective_status(observation.status, inventory.last_observed_at, time);
	InventoryChanged changes;
	changes.rearmed = rearmed;
	for (std::size_t i = 0; i < inventory.stations.size(); ++i)
		changes.changed_stations[i] = update_station(inventory.stations[i],
		                                             observation.stations[i],
		                                             inventory.observation,
		                                             time,
		                                             rearmed,
		                                             new_batch);
	if (changes.changed_stations.any() || rearmed)
	{
		if (inventory.revision == (std::numeric_limits<std::uint64_t>::max)())
			throw std::overflow_error("MC inventory revision exhausted.");
		++inventory.revision;
	}
	changes.revision = inventory.revision;
	return changes;
}

bool update_gun_ammunition(GunAmmunitionState& gun,
                           const GunAmmunitionObservation& observation,
                           std::chrono::nanoseconds time)
{
	const auto previous_count = gun.count;
	const auto previous_source = gun.source;
	const auto previous_status = gun.observation;
	gun.observation = observation.status;
	if (gun.observation.available && (observation.count < 0 || observation.count > 9999))
		gun.observation = { false, observation.status.revision, ObservationInvalidReason::InvalidNumeric };
	if (gun.observation.available && observation.status.revision != 0 &&
	    (!gun.last_observed_at || observation.status.revision != gun.last_observed_revision))
	{
		gun.count = observation.count;
		gun.source = AmmunitionSource::Native;
		gun.last_observed_at = time;
		gun.last_observed_revision = observation.status.revision;
	}
	gun.observation = effective_status(gun.observation, gun.last_observed_at, time);
	const bool changed = previous_count != gun.count || previous_source != gun.source ||
	                     previous_status.available != gun.observation.available ||
	                     previous_status.invalid_reason != gun.observation.invalid_reason;
	if (changed)
	{
		if (gun.revision == (std::numeric_limits<std::uint64_t>::max)())
			throw std::overflow_error("MC gun ammunition revision exhausted.");
		++gun.revision;
	}
	return changed;
}
}
