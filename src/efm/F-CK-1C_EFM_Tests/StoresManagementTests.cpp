#include "TestHarness.h"

#include "Core/Systems/MissionComputer/Components/StoresManagement/StoresManagement.h"
#include "Core/Systems/MissionComputer/Common/StoresQueries.h"

#include <chrono>
#include <limits>
#include <stdexcept>
#include <vector>

namespace
{
using namespace Core;
using namespace Core::Systems::Mc;
using namespace std::chrono;

struct Fixture
{
	MissionInputs input;
	StoresData stores;
	StoresEvents events;
	ModeData mode;
	StoresManagement manager{ input, stores, events };

	void select(OperatingMode selected)
	{
		const auto previous = mode;
		mode.operating_mode = selected;
		manager.on_operating_mode_selected({ previous, mode, selected != OperatingMode::Navigation });
	}

	void observe(std::uint64_t revision)
	{
		input.cockpit.weapon_stations.status = { true, revision, ObservationInvalidReason::None };
		for (auto& station : input.cockpit.weapon_stations.stations)
			if (station.configuration == StoreConfiguration::Unknown)
				station.configuration = StoreConfiguration::Empty;
	}

	void step(nanoseconds time)
	{
		input.observed_at = time;
		manager.step(time, 0.03125);
	}
};

void test_all_station_inventory_and_queries(Tests::Context& context)
{
	Fixture f;
	for (std::size_t i = 0; i < f.stores.inventory.stations.size(); ++i)
	{
		TEST_EXPECT(context, f.stores.inventory.stations[i].station_index == static_cast<int>(i));
		TEST_EXPECT(context, !f.stores.inventory.stations[i].last_observed_at);
	}
	TEST_EXPECT(context, !count_missiles(f.stores.inventory).complete);
	TEST_EXPECT(context, find_missile_stations(f.stores.inventory).empty());
	f.observe(2);
	auto& samples = f.input.cockpit.weapon_stations.stations;
	samples[0] = {
		StoreConfiguration::Equipped, 2, WeaponCategory::Missile, WeaponRole::AirToAir, "opaque-aa", "4.4.7.42", "RT"
	};
	samples[1] = {
		StoreConfiguration::Equipped, 1, WeaponCategory::Missile, WeaponRole::Other, "opaque-ag", "4.4.8.99", "RO"
	};
	samples[2] = { StoreConfiguration::Equipped, 3, WeaponCategory::Other, WeaponRole::Other, "bomb" };
	samples[3] = { StoreConfiguration::Equipped,
		           1,
		           WeaponCategory::Other,
		           WeaponRole::Other,
		           "AIM-9-in-a-non-missile-name" };
	samples[8] = {
		StoreConfiguration::Equipped, 1, WeaponCategory::Missile, WeaponRole::Unknown, "", "4.4.0.12", "LT"
	};
	f.step(seconds(1));
	const auto all = find_missile_stations(f.stores.inventory);
	TEST_EXPECT(context, (all == std::vector<int>{ 0, 1, 8 }));
	TEST_EXPECT(context, count_missiles(f.stores.inventory).count == 4);
	TEST_EXPECT(context, count_missiles(f.stores.inventory).complete);
	TEST_EXPECT(context, count_missiles(f.stores.inventory, { WeaponRole::Unknown, {} }).count == 1);
	TEST_EXPECT(context, count_missiles(f.stores.inventory, { WeaponRole::Unknown, {} }).complete);
	TEST_EXPECT(context, count_missiles(f.stores.inventory, { WeaponRole::AirToAir, {} }).count == 2);
	TEST_EXPECT(context, !count_missiles(f.stores.inventory, { WeaponRole::AirToAir, {} }).complete);
	TEST_EXPECT(context, (find_missile_stations(f.stores.inventory, { {}, "opaque-ag" }) == std::vector<int>{ 1 }));
	TEST_EXPECT(context, f.stores.inventory.stations[8].station_name == "LT");
	TEST_EXPECT(context, f.stores.inventory.stations[8].weapon_type_identifier == "4.4.0.12");
	TEST_EXPECT(context, f.stores.inventory.stations[2].count == 3);
	TEST_EXPECT(context, !f.stores.selection.requested_station);
	TEST_EXPECT(context, f.stores.selection.request_revision == 0);
}

void test_repeated_and_new_observations(Tests::Context& context)
{
	Fixture f;
	f.observe(2);
	f.input.cockpit.weapon_stations.stations[0] = { StoreConfiguration::Equipped,
		                                            2,
		                                            WeaponCategory::Missile,
		                                            WeaponRole::AirToAir,
		                                            "opaque" };
	std::vector<InventoryChanged> delivered;
	sigslot::scoped_connection connection = f.events.inventory_changed.connect(
	    [&](const InventoryChanged& event)
	    {
		    TEST_EXPECT(context, f.stores.inventory.revision == event.revision);
		    TEST_EXPECT(context, count_missiles(f.stores.inventory).count == f.stores.inventory.stations[0].count);
		    delivered.push_back(event);
	    });
	f.step(seconds(1));
	const auto revision = f.stores.inventory.revision;
	const auto load = f.stores.inventory.stations[0].load_revision;
	TEST_EXPECT(context, delivered.size() == 1);
	f.step(milliseconds(1100));
	TEST_EXPECT(context, f.stores.inventory.last_observed_at == seconds(1));
	TEST_EXPECT(context, f.stores.inventory.stations[0].last_observed_at == seconds(1));
	TEST_EXPECT(context, f.stores.inventory.revision == revision);
	TEST_EXPECT(context, delivered.size() == 1);
	f.observe(4);
	f.step(milliseconds(1200));
	TEST_EXPECT(context, f.stores.inventory.last_observed_at == milliseconds(1200));
	TEST_EXPECT(context, f.stores.inventory.revision == revision);
	TEST_EXPECT(context, delivered.size() == 1);
	f.input.cockpit.weapon_stations.stations[0].count = 1;
	f.observe(6);
	f.step(milliseconds(1300));
	TEST_EXPECT(context, f.stores.inventory.stations[0].count == 1);
	TEST_EXPECT(context, f.stores.inventory.stations[0].load_revision == load);
	TEST_EXPECT(context, delivered.size() == 2);
	TEST_EXPECT(context, delivered.back().changed_stations.count() == 1);
	TEST_EXPECT(context, delivered.back().changed_stations[0]);
}

void test_rearm_replacement_and_fixed_empty_stations(Tests::Context& context)
{
	Fixture f;
	f.observe(2);
	auto& observation = f.input.cockpit.weapon_stations;
	observation.stations[8] = {
		StoreConfiguration::Equipped, 1, WeaponCategory::Missile, WeaponRole::Other, "A", "4.4.8.1", "LT"
	};
	f.step(seconds(1));
	const auto initial = f.stores.inventory.stations[8].load_revision;
	observation.stations[8].weapon_identifier = "B";
	f.observe(4);
	f.step(seconds(2));
	TEST_EXPECT(context, f.stores.inventory.stations[8].load_revision == initial + 1);
	observation.rearm_revision = 1;
	f.observe(6);
	f.step(seconds(3));
	TEST_EXPECT(context, f.stores.inventory.stations[8].load_revision == initial + 2);
	f.step(seconds(4));
	TEST_EXPECT(context, f.stores.inventory.stations[8].load_revision == initial + 2);
	observation.stations[8].count = 0;
	f.observe(8);
	f.step(seconds(5));
	TEST_EXPECT(context, f.stores.inventory.stations[8].configuration == StoreConfiguration::Equipped);
	TEST_EXPECT(context, f.stores.inventory.stations[8].load_revision == initial + 2);
	TEST_EXPECT(context, find_missile_stations(f.stores.inventory).empty());
	observation.stations[8] = { StoreConfiguration::Empty };
	observation.stations[8].station_name = "LT";
	f.observe(10);
	f.step(seconds(6));
	TEST_EXPECT(context, f.stores.inventory.stations[8].configuration == StoreConfiguration::Empty);
	TEST_EXPECT(context, f.stores.inventory.stations[8].station_index == 8);
	TEST_EXPECT(context, f.stores.inventory.stations[8].station_name == "LT");
	TEST_EXPECT(context, f.stores.inventory.stations.size() == 9);
}

void test_unavailable_retains_last_known_data(Tests::Context& context)
{
	Fixture f;
	f.observe(2);
	f.input.cockpit.weapon_stations.stations[0] = { StoreConfiguration::Equipped,
		                                            2,
		                                            WeaponCategory::Missile,
		                                            WeaponRole::Other,
		                                            "opaque" };
	f.step(seconds(1));
	const auto load = f.stores.inventory.stations[0].load_revision;
	f.input.cockpit.weapon_stations = {};
	f.input.cockpit.weapon_stations.status = { false, 4, ObservationInvalidReason::StationApiError };
	f.step(seconds(2));
	TEST_EXPECT(context, f.stores.inventory.stations[0].count == 2);
	TEST_EXPECT(context, f.stores.inventory.stations[0].weapon_identifier == "opaque");
	TEST_EXPECT(context, f.stores.inventory.stations[0].last_observed_at == seconds(1));
	TEST_EXPECT(context, !f.stores.inventory.stations[0].observation.available);
	TEST_EXPECT(context,
	            f.stores.inventory.stations[0].observation.invalid_reason == ObservationInvalidReason::StationApiError);
	TEST_EXPECT(context, find_missile_stations(f.stores.inventory).empty());
	TEST_EXPECT(context, !count_missiles(f.stores.inventory).complete);
	f.observe(6);
	f.input.cockpit.weapon_stations.stations[0] = { StoreConfiguration::Equipped,
		                                            2,
		                                            WeaponCategory::Missile,
		                                            WeaponRole::Other,
		                                            "opaque" };
	f.step(seconds(3));
	TEST_EXPECT(context, count_missiles(f.stores.inventory).count == 2);
	TEST_EXPECT(context, count_missiles(f.stores.inventory).complete);
	TEST_EXPECT(context, f.stores.inventory.stations[0].load_revision == load);
	TEST_EXPECT(context, f.stores.inventory.stations[0].last_observed_at == seconds(3));
}

void test_recovery_of_same_batch_preserves_observation_time(Tests::Context& context)
{
	Fixture f;
	f.observe(2);
	f.input.cockpit.weapon_stations.stations[0] = { StoreConfiguration::Equipped,
		                                            1,
		                                            WeaponCategory::Missile,
		                                            WeaponRole::AirToAir,
		                                            "opaque" };
	f.step(seconds(1));
	f.input.cockpit.weapon_stations.status = { false, 0, ObservationInvalidReason::ParameterUnavailable };
	f.step(seconds(2));
	TEST_EXPECT(context, f.stores.inventory.last_observed_revision == 2);
	f.observe(2);
	f.step(seconds(3));
	TEST_EXPECT(context, !f.stores.inventory.observation.available);
	TEST_EXPECT(context, f.stores.inventory.observation.invalid_reason == ObservationInvalidReason::Stale);
	TEST_EXPECT(context, f.stores.inventory.last_observed_at == seconds(1));
	TEST_EXPECT(context, f.stores.inventory.stations[0].last_observed_at == seconds(1));
	f.observe(4);
	f.step(seconds(4));
	TEST_EXPECT(context, f.stores.inventory.last_observed_at == seconds(4));
}

void test_unknown_classification_and_instance_isolation(Tests::Context& context)
{
	Fixture f;
	Fixture other;
	int events = 0;
	sigslot::scoped_connection connection = f.events.inventory_changed.connect(
	    [&](const InventoryChanged&)
	    {
		    ++events;
	    });
	other.observe(2);
	other.step(seconds(1));
	TEST_EXPECT(context, events == 0);
	f.observe(2);
	f.input.cockpit.weapon_stations.stations[0] = { StoreConfiguration::Equipped,
		                                            1,
		                                            WeaponCategory::Unknown,
		                                            WeaponRole::Unknown,
		                                            "unknown" };
	f.step(seconds(1));
	TEST_EXPECT(context, events == 1);
	TEST_EXPECT(context, f.stores.inventory.stations[0].weapon_identifier == "unknown");
	TEST_EXPECT(context, count_missiles(f.stores.inventory).count == 0);
	TEST_EXPECT(context, !count_missiles(f.stores.inventory).complete);
	connection.disconnect();
	f.input.cockpit.weapon_stations.stations[0].count = 2;
	f.step(seconds(2));
	TEST_EXPECT(context, events == 1);
}
void test_clean_empty_and_depleted_are_independent(Tests::Context& context)
{
	Fixture f;
	f.observe(2);
	auto& samples = f.input.cockpit.weapon_stations.stations;
	samples[0] = { StoreConfiguration::Clean, 0, WeaponCategory::Unknown, WeaponRole::Unknown, "<CLEAN>", "", "RT" };
	samples[1] = {
		StoreConfiguration::Equipped, 0, WeaponCategory::Missile, WeaponRole::AirToAir, "depleted", "4.4.7.1", "RO"
	};
	f.step(seconds(1));
	TEST_EXPECT(context, f.stores.inventory.stations[0].configuration == StoreConfiguration::Clean);
	TEST_EXPECT(context, f.stores.inventory.stations[1].configuration == StoreConfiguration::Equipped);
	TEST_EXPECT(context, f.stores.inventory.stations[2].configuration == StoreConfiguration::Empty);
	TEST_EXPECT(context, count_missiles(f.stores.inventory).count == 0);
	TEST_EXPECT(context, count_missiles(f.stores.inventory).complete);
	TEST_EXPECT(context, find_missile_stations(f.stores.inventory).empty());
	const auto revision = f.stores.inventory.revision;
	samples[0] = { StoreConfiguration::Empty, 0, WeaponCategory::Unknown, WeaponRole::Unknown, "", "", "RT" };
	f.observe(4);
	f.step(milliseconds(1100));
	TEST_EXPECT(context, f.stores.inventory.stations[0].configuration == StoreConfiguration::Empty);
	TEST_EXPECT(context, f.stores.inventory.revision == revision + 1);
}

void test_native_gun_count_freshness_and_notifications(Tests::Context& context)
{
	Fixture f;
	TEST_EXPECT(context, !f.stores.gun_ammunition.count);
	std::vector<GunAmmunitionChanged> delivered;
	sigslot::scoped_connection connection = f.events.gun_ammunition_changed.connect(
	    [&](const GunAmmunitionChanged& event)
	    {
		    TEST_EXPECT(context, f.stores.gun_ammunition.revision == event.revision);
		    delivered.push_back(event);
	    });
	auto& sample = f.input.cockpit.gun_ammunition;
	sample = { { true, 2, ObservationInvalidReason::None }, 261 };
	f.step(seconds(1));
	TEST_EXPECT(context, f.stores.gun_ammunition.count == 261);
	TEST_EXPECT(context, f.stores.gun_ammunition.source == AmmunitionSource::Native);
	TEST_EXPECT(context, f.stores.gun_ammunition.observation.available);
	TEST_EXPECT(context, delivered.size() == 1);
	f.step(milliseconds(1100));
	TEST_EXPECT(context, f.stores.gun_ammunition.last_observed_at == seconds(1));
	TEST_EXPECT(context, delivered.size() == 1);
	sample.status.revision = 4;
	f.step(milliseconds(1400));
	TEST_EXPECT(context, f.stores.gun_ammunition.last_observed_at == milliseconds(1400));
	TEST_EXPECT(context, delivered.size() == 1);
	f.step(milliseconds(1901));
	TEST_EXPECT(context, !f.stores.gun_ammunition.observation.available);
	TEST_EXPECT(context, f.stores.gun_ammunition.observation.invalid_reason == ObservationInvalidReason::Stale);
	TEST_EXPECT(context, f.stores.gun_ammunition.count == 261);
	TEST_EXPECT(context, delivered.size() == 2);
	sample = { { true, 6, ObservationInvalidReason::None }, 216 };
	f.step(seconds(2));
	TEST_EXPECT(context, f.stores.gun_ammunition.observation.available);
	TEST_EXPECT(context, f.stores.gun_ammunition.count == 216);
	sample = { { true, 8, ObservationInvalidReason::None }, 0 };
	f.step(milliseconds(2100));
	TEST_EXPECT(context, f.stores.gun_ammunition.observation.available);
	TEST_EXPECT(context, f.stores.gun_ammunition.count == 0);
	sample = { { true, 10, ObservationInvalidReason::None }, 523 };
	f.step(milliseconds(2200));
	TEST_EXPECT(context, f.stores.gun_ammunition.count == 523);
	sample = { { false, 12, ObservationInvalidReason::NotProvided }, 0 };
	f.step(milliseconds(2300));
	TEST_EXPECT(context, !f.stores.gun_ammunition.observation.available);
	TEST_EXPECT(context, f.stores.gun_ammunition.count == 523);
	const auto last_valid = f.stores.gun_ammunition.last_observed_at;
	sample = { { true, 14, ObservationInvalidReason::None }, -1 };
	f.step(milliseconds(2400));
	TEST_EXPECT(context, !f.stores.gun_ammunition.observation.available);
	TEST_EXPECT(context,
	            f.stores.gun_ammunition.observation.invalid_reason == ObservationInvalidReason::InvalidNumeric);
	TEST_EXPECT(context, f.stores.gun_ammunition.count == 523);
	TEST_EXPECT(context, f.stores.gun_ammunition.last_observed_at == last_valid);
	Fixture other;
	TEST_EXPECT(context, !other.stores.gun_ammunition.count);
}

void test_station_freshness_retains_observations_and_recovers(Tests::Context& context)
{
	Fixture f;
	f.observe(2);
	auto& sample = f.input.cockpit.weapon_stations.stations[8];
	sample = { StoreConfiguration::Equipped, 2, WeaponCategory::Missile, WeaponRole::Other, "opaque", "4.4.8.9", "LT" };
	f.step(seconds(1));
	const auto load = f.stores.inventory.stations[8].load_revision;
	const auto revision = f.stores.inventory.revision;
	f.step(milliseconds(1500));
	TEST_EXPECT(context, f.stores.inventory.observation.available);
	f.step(milliseconds(1501));
	TEST_EXPECT(context, !f.stores.inventory.observation.available);
	TEST_EXPECT(context, f.stores.inventory.observation.invalid_reason == ObservationInvalidReason::Stale);
	TEST_EXPECT(context, f.stores.inventory.stations[8].count == 2);
	TEST_EXPECT(context, f.stores.inventory.stations[8].configuration == StoreConfiguration::Equipped);
	TEST_EXPECT(context, f.stores.inventory.stations[8].last_observed_at == seconds(1));
	TEST_EXPECT(context, f.stores.inventory.revision == revision + 1);
	TEST_EXPECT(context, !count_missiles(f.stores.inventory).complete);
	f.step(milliseconds(1600));
	TEST_EXPECT(context, f.stores.inventory.revision == revision + 1);
	f.observe(4);
	f.step(milliseconds(1700));
	TEST_EXPECT(context, f.stores.inventory.observation.available);
	TEST_EXPECT(context, count_missiles(f.stores.inventory).count == 2);
	TEST_EXPECT(context, f.stores.inventory.stations[8].load_revision == load);
	TEST_EXPECT(context, f.stores.inventory.last_observed_at == milliseconds(1700));
	TEST_EXPECT(context, f.stores.inventory.revision == revision + 2);
	// 同批次的內容沿用最近已接受的觀測。
	sample.count = 99;
	f.step(milliseconds(1800));
	TEST_EXPECT(context, f.stores.inventory.stations[8].count == 2);
	f.observe(0);
	f.step(milliseconds(1900));
	TEST_EXPECT(context, !f.stores.inventory.observation.available);
	TEST_EXPECT(context, f.stores.inventory.observation.invalid_reason == ObservationInvalidReason::InvalidRevision);
}

void test_selection_cycles_candidates_and_keeps_inventory_authority(Tests::Context& context)
{
	Fixture f;
	f.observe(2);
	auto& samples = f.input.cockpit.weapon_stations.stations;
	samples[0] = { StoreConfiguration::Equipped, 1, WeaponCategory::Missile, WeaponRole::Other, "ag" };
	for (int i : { 2, 5, 8 })
		samples[i] = { StoreConfiguration::Equipped, 1, WeaponCategory::Missile, WeaponRole::AirToAir, "aa" };
	f.select(OperatingMode::Dogfight);
	f.step(seconds(1));
	TEST_EXPECT(context, f.stores.selection.requested_station == 2);
	TEST_EXPECT(context, f.stores.selection.request_revision == 1);
	TEST_EXPECT(context, f.stores.selection.pending());
	const auto inventory_revision = f.stores.inventory.revision;
	for (int expected : { 5, 8, 2 })
	{
		f.select(OperatingMode::Dogfight);
		f.step(milliseconds(1100));
		TEST_EXPECT(context, f.stores.selection.requested_station == expected);
	}
	TEST_EXPECT(context, f.stores.inventory.revision == inventory_revision);
	TEST_EXPECT(context, count_missiles(f.stores.inventory).count == 4);
	f.select(OperatingMode::Dogfight);
	f.step(milliseconds(1200));
	TEST_EXPECT(context, f.stores.selection.requested_station == 5);
	f.step(milliseconds(1250));
	TEST_EXPECT(context, f.stores.selection.requested_station == 5);
	f.select(OperatingMode::MissileOverride);
	f.step(milliseconds(1300));
	TEST_EXPECT(context, f.stores.selection.requested_station == 2);
	f.select(OperatingMode::Navigation);
	f.step(milliseconds(1400));
	TEST_EXPECT(context, !f.stores.selection.requested_station);
	TEST_EXPECT(context, count_missiles(f.stores.inventory).count == 4);
}

void test_selection_acknowledgements_and_completed_notifications(Tests::Context& context)
{
	Fixture f;
	f.observe(2);
	auto& observation = f.input.cockpit.weapon_stations;
	observation.stations[8] = { StoreConfiguration::Equipped, 2, WeaponCategory::Missile, WeaponRole::AirToAir, "aa" };
	std::vector<SelectionChanged> delivered;
	sigslot::scoped_connection connection = f.events.selection_changed.connect(
	    [&](const SelectionChanged& event)
	    {
		    TEST_EXPECT(context, event.request_revision == f.stores.selection.request_revision);
		    TEST_EXPECT(context, event.acknowledged_revision == f.stores.selection.acknowledged_revision);
		    TEST_EXPECT(context, count_missiles(f.stores.inventory).count == 2);
		    delivered.push_back(event);
	    });
	f.select(OperatingMode::MissileOverride);
	f.step(seconds(1));
	TEST_EXPECT(context, delivered.size() == 1);
	observation.selected_station = 8;
	observation.selection_ack_revision = 2;
	f.step(milliseconds(1100));
	TEST_EXPECT(context, f.stores.selection.pending());
	observation.selection_ack_revision = 1;
	observation.selected_station = 2;
	f.step(milliseconds(1200));
	TEST_EXPECT(context, f.stores.selection.pending());
	observation.selected_station = 8;
	f.step(milliseconds(1300));
	TEST_EXPECT(context, !f.stores.selection.pending());
	TEST_EXPECT(context, f.stores.selection.acknowledged_station == 8);
	TEST_EXPECT(context, delivered.size() == 2);
	f.select(OperatingMode::MissileOverride);
	f.step(milliseconds(1400));
	TEST_EXPECT(context, f.stores.selection.requested_station == 8);
	TEST_EXPECT(context, f.stores.selection.request_revision == 2);
	TEST_EXPECT(context, f.stores.selection.acknowledged_revision == 1);
	TEST_EXPECT(context, f.stores.selection.pending());
	TEST_EXPECT(context, observation.stations[8].count == 2);
}

void test_selection_waits_for_inventory_and_reselects_replaced_loads(Tests::Context& context)
{
	Fixture f;
	f.select(OperatingMode::Dogfight);
	f.select(OperatingMode::Dogfight);
	f.step(seconds(1));
	TEST_EXPECT(context, f.stores.selection.request_revision == 0);
	f.observe(2);
	auto& observation = f.input.cockpit.weapon_stations;
	for (int i : { 2, 8 })
		observation.stations[i] = { StoreConfiguration::Equipped,
			                        1,
			                        WeaponCategory::Missile,
			                        WeaponRole::AirToAir,
			                        "aa" };
	f.step(milliseconds(1100));
	TEST_EXPECT(context, f.stores.selection.requested_station == 8);
	const auto first = f.stores.selection.request_revision;
	observation.stations[8].weapon_identifier = "another-aa";
	f.observe(4);
	f.step(milliseconds(1200));
	TEST_EXPECT(context, f.stores.selection.requested_station == 8);
	TEST_EXPECT(context, f.stores.selection.request_revision == first + 1);
	observation.rearm_revision = 1;
	f.observe(6);
	f.step(milliseconds(1300));
	TEST_EXPECT(context, f.stores.selection.request_revision == first + 2);
	f.step(milliseconds(1900));
	TEST_EXPECT(context, !f.stores.inventory.observation.available);
	TEST_EXPECT(context, f.stores.selection.requested_station == 8);
	f.select(OperatingMode::Dogfight);
	f.step(milliseconds(1950));
	TEST_EXPECT(context, f.stores.selection.request_revision == first + 2);
	f.select(OperatingMode::Navigation);
	f.step(seconds(2));
	TEST_EXPECT(context, !f.stores.selection.requested_station);
	f.select(OperatingMode::Dogfight);
	f.observe(8);
	f.step(milliseconds(2100));
	TEST_EXPECT(context, f.stores.selection.requested_station == 2);
	observation.stations[2].count = 0;
	f.observe(10);
	f.step(milliseconds(2200));
	TEST_EXPECT(context, f.stores.selection.requested_station == 8);
	observation.stations[8].count = 0;
	f.observe(12);
	f.step(milliseconds(2300));
	TEST_EXPECT(context, !f.stores.selection.requested_station);
	TEST_EXPECT(context, count_missiles(f.stores.inventory).complete);
	TEST_EXPECT(context, count_missiles(f.stores.inventory).count == 0);
}

}

void run_stores_management_tests(Tests::Context& context)
{
	test_station_freshness_retains_observations_and_recovers(context);
	test_selection_cycles_candidates_and_keeps_inventory_authority(context);
	test_selection_acknowledgements_and_completed_notifications(context);
	test_selection_waits_for_inventory_and_reselects_replaced_loads(context);
	test_clean_empty_and_depleted_are_independent(context);
	test_native_gun_count_freshness_and_notifications(context);
	test_all_station_inventory_and_queries(context);
	test_repeated_and_new_observations(context);
	test_rearm_replacement_and_fixed_empty_stations(context);
	test_unavailable_retains_last_known_data(context);
	test_recovery_of_same_batch_preserves_observation_time(context);
	test_unknown_classification_and_instance_isolation(context);
}
