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
	WeaponOperationData weapon;
	StoresData stores;
	ExternalActionQueue actions;
	StoresEvents events;
	StoresManagement manager{ input, weapon, stores, actions, events };

	void observe(std::uint64_t revision)
	{
		input.cockpit.weapon_stations.status = { true, revision, ObservationInvalidReason::None };
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
	samples[0] = { true, 2, WeaponCategory::Missile, WeaponRole::AirToAir, "opaque-aa", "4.4.7.42", "RT" };
	samples[1] = { true, 1, WeaponCategory::Missile, WeaponRole::Other, "opaque-ag", "4.4.8.99", "RO" };
	samples[2] = { true, 3, WeaponCategory::Other, WeaponRole::Other, "bomb" };
	samples[3] = { true, 1, WeaponCategory::Other, WeaponRole::Other, "AIM-9-in-a-non-missile-name" };
	samples[8] = { true, 1, WeaponCategory::Missile, WeaponRole::Unknown, "", "4.4.0.12", "LT" };
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
	TEST_EXPECT(context, !f.stores.requested_station);
	TEST_EXPECT(context, f.actions.batch().count == 0);
}

void test_repeated_and_new_observations(Tests::Context& context)
{
	Fixture f;
	f.observe(2);
	f.input.cockpit.weapon_stations.stations[0] = { true, 2, WeaponCategory::Missile, WeaponRole::AirToAir, "opaque" };
	std::vector<InventoryChanged> delivered;
	sigslot::scoped_connection connection = f.events.inventory_changed.connect(
	    [&](const InventoryChanged& event)
	    {
		    TEST_EXPECT(context, f.stores.inventory.revision == event.revision);
		    TEST_EXPECT(context, f.stores.missile_count == f.stores.inventory.stations[0].count);
		    delivered.push_back(event);
	    });
	f.step(seconds(1));
	const auto revision = f.stores.inventory.revision;
	const auto load = f.stores.inventory.stations[0].load_revision;
	TEST_EXPECT(context, delivered.size() == 1);
	f.step(seconds(2));
	TEST_EXPECT(context, f.stores.inventory.last_observed_at == seconds(1));
	TEST_EXPECT(context, f.stores.inventory.stations[0].last_observed_at == seconds(1));
	TEST_EXPECT(context, f.stores.inventory.revision == revision);
	TEST_EXPECT(context, delivered.size() == 1);
	f.observe(4);
	f.step(seconds(3));
	TEST_EXPECT(context, f.stores.inventory.last_observed_at == seconds(3));
	TEST_EXPECT(context, f.stores.inventory.revision == revision);
	TEST_EXPECT(context, delivered.size() == 1);
	f.input.cockpit.weapon_stations.stations[0].count = 1;
	f.observe(6);
	f.step(seconds(4));
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
	observation.stations[8] = { true, 1, WeaponCategory::Missile, WeaponRole::Other, "A", "4.4.8.1", "LT" };
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
	TEST_EXPECT(context, f.stores.inventory.stations[8].present);
	TEST_EXPECT(context, f.stores.inventory.stations[8].load_revision == initial + 2);
	TEST_EXPECT(context, find_missile_stations(f.stores.inventory).empty());
	observation.stations[8] = {};
	observation.stations[8].station_name = "LT";
	f.observe(10);
	f.step(seconds(6));
	TEST_EXPECT(context, !f.stores.inventory.stations[8].present);
	TEST_EXPECT(context, f.stores.inventory.stations[8].station_index == 8);
	TEST_EXPECT(context, f.stores.inventory.stations[8].station_name == "LT");
	TEST_EXPECT(context, f.stores.inventory.stations.size() == 9);
}

void test_unavailable_retains_last_known_data(Tests::Context& context)
{
	Fixture f;
	f.observe(2);
	f.input.cockpit.weapon_stations.stations[0] = { true, 2, WeaponCategory::Missile, WeaponRole::Other, "opaque" };
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
	f.input.cockpit.weapon_stations.stations[0] = { true, 2, WeaponCategory::Missile, WeaponRole::Other, "opaque" };
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
	f.input.cockpit.weapon_stations.stations[0] = { true, 1, WeaponCategory::Missile, WeaponRole::AirToAir, "opaque" };
	f.step(seconds(1));
	f.input.cockpit.weapon_stations.status = { false, 0, ObservationInvalidReason::ParameterUnavailable };
	f.step(seconds(2));
	TEST_EXPECT(context, f.stores.inventory.last_observed_revision == 2);
	f.observe(2);
	f.step(seconds(3));
	TEST_EXPECT(context, f.stores.inventory.observation.available);
	TEST_EXPECT(context, f.stores.inventory.last_observed_at == seconds(1));
	TEST_EXPECT(context, f.stores.inventory.stations[0].last_observed_at == seconds(1));
	f.observe(4);
	f.step(seconds(4));
	TEST_EXPECT(context, f.stores.inventory.last_observed_at == seconds(4));
}

void test_estimates_and_selection_do_not_rewrite_inventory(Tests::Context& context)
{
	Fixture f;
	f.observe(2);
	f.input.cockpit.weapon_stations.stations[8] = { true, 2, WeaponCategory::Missile, WeaponRole::AirToAir, "opaque" };
	f.step(seconds(1));
	f.manager.record_missile_release();
	TEST_EXPECT(context, f.stores.inventory.stations[8].count == 2);
	f.manager.request_station(8);
	TEST_EXPECT(context, f.stores.requested_station == 8);
	TEST_EXPECT(context, !f.stores.selected_station);
	auto& observation = f.input.cockpit.weapon_stations;
	observation.selected_station = 8;
	observation.selection_ack_revision = f.stores.selection_request_revision + 1;
	f.step(seconds(2));
	TEST_EXPECT(context, !f.stores.selected_station);
	observation.selection_ack_revision = f.stores.selection_request_revision;
	f.step(seconds(3));
	TEST_EXPECT(context, f.stores.selected_station == 8);
	bool rejected = false;
	try
	{
		f.manager.request_station(9);
	}
	catch (const std::out_of_range&)
	{
		rejected = true;
	}
	TEST_EXPECT(context, rejected);
	TEST_EXPECT(context, f.stores.requested_station == 8);
	f.input.cockpit.weapon_stations.status.available = false;
	f.step(seconds(4));
	f.manager.record_missile_release();
	TEST_EXPECT(context, f.stores.missile_count == 1);
	TEST_EXPECT(context, f.stores.inventory.stations[8].count == 2);
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
	f.input.cockpit.weapon_stations.stations[0] = { true, 1, WeaponCategory::Unknown, WeaponRole::Unknown, "unknown" };
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
}

void run_stores_management_tests(Tests::Context& context)
{
	test_all_station_inventory_and_queries(context);
	test_repeated_and_new_observations(context);
	test_rearm_replacement_and_fixed_empty_stations(context);
	test_unavailable_retains_last_known_data(context);
	test_recovery_of_same_batch_preserves_observation_time(context);
	test_estimates_and_selection_do_not_rewrite_inventory(context);
	test_unknown_classification_and_instance_isolation(context);
}
