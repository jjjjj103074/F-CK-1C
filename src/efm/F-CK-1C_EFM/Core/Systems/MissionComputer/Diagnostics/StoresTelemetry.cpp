#include "StoresTelemetry.h"

#include "../Common/StoresQueries.h"
#include "../../SystemPipeline.h"

#include <algorithm>

namespace
{
using namespace Core;
using namespace Core::Systems;
using namespace Core::Systems::Mc;

template <typename T>
DebugTelemetryChannel<T> declare(SystemSetup& setup, const std::string& name, const std::string& label)
{
	return setup.declare_debug_channel<T>(
	    { "mc_stores_" + name, label, DebugTelemetryType<T>::value, "", "MC stores observation diagnostics." });
}

const char* category_name(WeaponCategory category)
{
	switch (category)
	{
	case WeaponCategory::Missile:
		return "MSL";
	case WeaponCategory::Other:
		return "OTHER";
	default:
		return "UNKNOWN";
	}
}

const char* role_name(WeaponRole role)
{
	switch (role)
	{
	case WeaponRole::AirToAir:
		return "AA";
	case WeaponRole::Other:
		return "OTHER";
	default:
		return "UNKNOWN";
	}
}

std::string identity(const std::string& type, const std::string& clsid)
{
	return "TYPE=" + type + ";CLSID=" + clsid;
}

std::string station_input(const WeaponStationSample& sample)
{
	return sample.station_name + " P=" + (sample.present ? "1" : "0") + " Q=" + std::to_string(sample.count) + ";" +
	       identity(sample.weapon_type_identifier, sample.weapon_identifier);
}

std::string station_state(const StoreStationState& station)
{
	return station.station_name + " P=" + (station.present ? "1" : "0") + " Q=" + std::to_string(station.count) + " " +
	       category_name(station.category) + "/" + role_name(station.role) +
	       " L=" + std::to_string(station.load_revision) + " OK=" + (station.observation.available ? "1" : "0");
}
}

namespace Core::Systems::Mc
{
void StoresTelemetry::setup(SystemSetup& setup)
{
	observation_available_ = declare<bool>(setup, "observation_available", "STORES OBS OK");
	observation_reason_ = declare<std::int64_t>(setup, "observation_reason", "STORES OBS REASON");
	observation_revision_ = declare<std::string>(setup, "observation_revision", "STORES SOURCE BATCH");
	inventory_revision_ = declare<std::string>(setup, "inventory_revision", "STORES INVENTORY REV");
	rearm_revision_ = declare<std::string>(setup, "rearm_revision", "STORES REARM REV");
	observation_age_s_ = declare<double>(setup, "observation_age_s", "STORES AGE S");
	missile_count_ = declare<std::int64_t>(setup, "missile_count", "ALL MISSILES");
	missile_count_complete_ = declare<bool>(setup, "missile_count_complete", "ALL COUNT COMPLETE");
	aa_count_ = declare<std::int64_t>(setup, "aa_count", "AA MISSILES");
	aa_count_complete_ = declare<bool>(setup, "aa_count_complete", "AA COUNT COMPLETE");
	aa_summary_count_ = declare<std::int64_t>(setup, "aa_summary_count", "AA DISPLAY COUNT");
	aa_summary_source_ = declare<std::int64_t>(setup, "aa_summary_source", "AA DISPLAY SOURCE");
	gun_api_available_ = declare<bool>(setup, "gun_api_available", "GUN API OK");
	gun_api_count_ = declare<std::int64_t>(setup, "gun_api_count", "GUN API COUNT");
	gun_count_ = declare<double>(setup, "gun_count", "GUN MC COUNT");
	gun_requested_ = declare<bool>(setup, "gun_requested", "GUN REQUESTED");
	for (std::size_t i = 0; i < stations_.size(); ++i)
	{
		const auto number = (i < 9 ? "0" : "") + std::to_string(i + 1);
		const auto name = "station_" + number;
		stations_[i].state = declare<std::string>(setup, name, "MC " + number);
		stations_[i].identity = declare<std::string>(setup, name + "_identity", "ID " + number);
		stations_[i].input = declare<std::string>(setup, name + "_input", "IN " + number);
	}
}

void StoresTelemetry::publish(DebugSimulationTime time,
                              const MissionInputs& input,
                              const StoresData& stores,
                              const WeaponOperationData& weapon) const
{
	const auto& source = input.cockpit.weapon_stations;
	const auto& inventory = stores.inventory;
	const auto all = count_missiles(inventory);
	const auto aa = count_missiles(inventory, { WeaponRole::AirToAir, {} });
	const double age_s =
	    inventory.last_observed_at
	        ? std::chrono::duration<double>((std::max)(time, input.observed_at) - *inventory.last_observed_at).count()
	        : -1.0;
	observation_available_.publish(time, source.status.available);
	observation_reason_.publish(time, static_cast<std::int64_t>(source.status.invalid_reason));
	observation_revision_.publish(time, std::to_string(source.status.revision));
	inventory_revision_.publish(time, std::to_string(inventory.revision));
	rearm_revision_.publish(time, std::to_string(inventory.rearm_revision));
	observation_age_s_.publish(time, age_s);
	missile_count_.publish(time, all.count);
	missile_count_complete_.publish(time, all.complete);
	aa_count_.publish(time, aa.count);
	aa_count_complete_.publish(time, aa.complete);
	aa_summary_count_.publish(time, stores.missile_count);
	aa_summary_source_.publish(time, static_cast<std::int64_t>(stores.count_source));
	gun_api_available_.publish(time, input.cockpit.mission_adapter.gun_count_available);
	gun_api_count_.publish(time, input.cockpit.mission_adapter.gun_count);
	gun_count_.publish(time, stores.gun_count);
	gun_requested_.publish(time, weapon.gun_requested);
	for (std::size_t i = 0; i < stations_.size(); ++i)
	{
		const auto& station = inventory.stations[i];
		stations_[i].state.publish(time, station_state(station));
		stations_[i].identity.publish(time, identity(station.weapon_type_identifier, station.weapon_identifier));
		stations_[i].input.publish(time, source.status.available ? station_input(source.stations[i]) : "UNAVAILABLE");
	}
}
}
