#include "SystemPipelineTestFixture.h"

#include "Core/Systems/MissionComputer/Components/Executive/Executive.h"
#include "Core/Systems/MissionComputer/Components/StartupAndRestart/StartupAndRestart.h"
#include "Core/Systems/MissionComputer/MissionComputer.h"

#include <algorithm>
#include <chrono>
#include <iterator>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace Core::Systems::Catalog::MissionComputer
{
SystemEntry create_entry();
}

namespace
{
using namespace Core;
using namespace Core::Systems;
using namespace Core::Systems::Mc;
using namespace std::chrono;

constexpr nanoseconds kMissionPeriod(31'250'000);

static_assert(!std::is_copy_constructible<MissionComputer>::value,
              "MC must preserve database and task target addresses.");
static_assert(!std::is_move_constructible<MissionComputer>::value,
              "Moving MC would invalidate constructor-bound references.");
static_assert(!std::is_copy_constructible<FireControl>::value,
              "FireControl owns a weapon operation bound to its internal targeting module.");
static_assert(!std::is_move_constructible<FireControl>::value,
              "Moving FireControl would invalidate its internal targeting reference.");

struct Call
{
	std::string id;
	nanoseconds time;
	double dt_s;
};

struct Recorder
{
	std::string id;
	std::vector<Call>& calls;

	void step(nanoseconds time, double dt_s)
	{
		calls.push_back({ id, time, dt_s });
	}
};

void test_startup_and_schedule(Tests::Context& context)
{
	std::vector<Call> calls;
	Recorder fast{ "fast", calls }, slow{ "slow", calls };
	ExecutiveTables tables;
	Executive executive(tables,
	                    { { "fast.step", TaskFrequency(4), fast, &Recorder::step },
	                      { "slow.step", TaskFrequency(2), slow, &Recorder::step } });
	LifecycleStatus lifecycle;
	StartupAndRestart startup(executive, lifecycle);
	TEST_EXPECT(context,
	            SystemPipelineTest::action_throws(
	                [&]
	                {
		                executive.step(seconds(1));
	                }));
	startup.start();
	TEST_EXPECT(context, calls.empty());
	executive.step(nanoseconds(0));
	TEST_EXPECT(context, calls.empty());
	executive.step(milliseconds(750));
	TEST_EXPECT(context, calls.size() == 4);
	executive.step(milliseconds(750));
	TEST_EXPECT(context, calls.size() == 4);
	executive.step(seconds(1));
	TEST_EXPECT(context, calls.size() == 6);
	if (calls.size() == 6)
	{
		TEST_EXPECT(context, calls[0].time == milliseconds(250));
		TEST_EXPECT(context, calls[1].id == "fast");
		TEST_EXPECT(context, calls[2].id == "slow");
		TEST_EXPECT(context, calls[1].time == milliseconds(500));
		TEST_EXPECT(context, calls[2].time == milliseconds(500));
		TEST_EXPECT_NEAR(context, calls[0].dt_s, 0.25, 1.0e-12);
		TEST_EXPECT_NEAR(context, calls[2].dt_s, 0.5, 1.0e-12);
	}
	TEST_EXPECT(context, tables.tasks()[0].state.completed_ticks == 4);
	TEST_EXPECT(context, tables.tasks()[1].state.completed_ticks == 2);
	TEST_EXPECT(context, tables.scheduler_state().advanced_through == seconds(1));
	TEST_EXPECT(context,
	            SystemPipelineTest::action_throws(
	                [&]
	                {
		                startup.start();
	                }));
	TEST_EXPECT(context,
	            SystemPipelineTest::action_throws(
	                [&]
	                {
		                executive.step(milliseconds(999));
	                }));
	TEST_EXPECT(context, tables.scheduler_state().advanced_through == seconds(1));
}

void test_default_assembly(Tests::Context& context)
{
	InputOutputData io;
	SystemStatusTables status;
	ExecutiveTables tables;
	SystemMonitor monitor(io, status.health);
	FailureManager failure(status.health, status.failure_response);
	Executive executive(tables, monitor, failure);
	StartupAndRestart startup(executive, status.lifecycle);
	TEST_EXPECT(context, !tables.scheduler_state().started);
	TEST_EXPECT(context, tables.tasks().size() == 2);
	startup.start();
	executive.step(seconds(1));
	TEST_EXPECT(context, tables.scheduler_state().started);
	TEST_EXPECT(context, tables.tasks()[0].definition.id == "system_monitor.step");
	TEST_EXPECT(context, tables.tasks()[1].definition.id == "failure_manager.step");
	TEST_EXPECT(context, tables.tasks()[0].state.completed_ticks == 32);
	TEST_EXPECT(context, tables.tasks()[1].state.completed_ticks == 32);
}

void test_entry_in_pipeline(Tests::Context& context)
{
	// MC uses the built-in FrameInput and publishes a typed snapshot without other Systems.
	SystemEntry entry = Catalog::MissionComputer::create_entry();
	TEST_EXPECT(context, entry.id == "mission_computer");
	SystemPipeline pipeline(SystemPipelineTest::flight_setup(), { entry });
	TEST_EXPECT(context, pipeline.system_count() == 1);
	const auto before = std::make_unique<const AircraftDataSnapshot>(
	    SystemPipelineTest::step_pipeline_to(pipeline, {}, {}, kMissionPeriod - nanoseconds(1)));
	TEST_EXPECT(context, !before->read(AircraftDataKeys::kMissionComputerSnapshot).status.available);
	const auto first =
	    std::make_unique<AircraftDataSnapshot>(SystemPipelineTest::step_pipeline_to(pipeline, {}, {}, kMissionPeriod));
	TEST_EXPECT(context, first->read(AircraftDataKeys::kMissionComputerSnapshot).status.revision == 1);
	const auto snapshot =
	    std::make_unique<AircraftDataSnapshot>(SystemPipelineTest::step_pipeline_to(pipeline, {}, {}, seconds(1)));
	TEST_EXPECT(context, snapshot->read(AircraftDataKeys::kMissionComputerSnapshot).status.revision == 32);
	TEST_EXPECT(context, pipeline.advanced_through() == seconds(1));
	TEST_EXPECT(context, !snapshot->has(AircraftDataKeys::kPilotControlSignal));
	TEST_EXPECT(context, !snapshot->has(AircraftDataKeys::kFuelData));
	TEST_EXPECT(context, snapshot->has(AircraftDataKeys::kMissionComputerSnapshot));
	TEST_EXPECT(context, snapshot->read(AircraftDataKeys::kMissionComputerSnapshot).status.available);
}

struct MissionFixture
{
	SystemPipeline pipeline{ SystemPipelineTest::flight_setup(), { Catalog::MissionComputer::create_entry() } };
	FrameInput frame;

	MissionFixture()
	{
		frame.cockpit.mission_adapter.available = true;
		frame.cockpit.mission_adapter.radar_power_on = true;
		frame.cockpit.weapon_stations.status = { true, 2, ObservationInvalidReason::None };
		for (auto& station : frame.cockpit.weapon_stations.stations)
			station.configuration = StoreConfiguration::Empty;
	}

	CombatAvionicsSnapshot step(nanoseconds target)
	{
		return SystemPipelineTest::step_pipeline_to(pipeline, frame, {}, target)
		    .read(AircraftDataKeys::kMissionComputerSnapshot);
	}

	void send(CommandId id, double value = 1.0)
	{
		pipeline.send({ id, value });
	}
};

bool contains(const MissionActionBatch& batch, MissionActionKind action)
{
	return std::find_if(batch.actions.begin(),
	                    batch.actions.begin() + batch.count,
	                    [action](const MissionAction& entry)
	                    {
		                    return entry.kind == action;
	                    }) != batch.actions.begin() + batch.count;
}

void test_action_delivery_retains_order_and_rejects_future_ack(Tests::Context& context)
{
	ExternalActionQueue queue;
	for (int i = 0; i < 40; ++i)
		queue.append(i % 2 == 0 ? MissionActionKind::GunOn : MissionActionKind::GunOff);
	queue.acknowledge(40); // Nothing has been published yet.
	MissionActionBatch batch = queue.batch();
	TEST_EXPECT(context, batch.count == kMissionActionWindowSize && batch.first_sequence == 1);
	queue.acknowledge(40); // Outside the transmitted window.
	TEST_EXPECT(context, queue.batch().first_sequence == 1);
	queue.acknowledge(16);
	batch = queue.batch();
	TEST_EXPECT(context, batch.count == 16 && batch.first_sequence == 17);
	queue.acknowledge(15);
	TEST_EXPECT(context, queue.batch().first_sequence == 17);
	queue.acknowledge(32);
	batch = queue.batch();
	TEST_EXPECT(context, batch.count == 8 && batch.first_sequence == 33);
	for (std::size_t i = 0; i < batch.count; ++i)
	{
		TEST_EXPECT(context,
		            batch.actions[i].kind == (i % 2 == 0 ? MissionActionKind::GunOn : MissionActionKind::GunOff));
	}
	queue.acknowledge(40);
	TEST_EXPECT(context, queue.batch().count == 0);
}

void test_short_trigger_waits_for_execution_and_does_not_backdate_ack(Tests::Context& context)
{
	MissionFixture fixture;
	fixture.step(kMissionPeriod);
	fixture.send(CommandId::SelectDogfightMode);
	fixture.send(CommandId::SetTriggerSecondStage);
	fixture.send(CommandId::SetTriggerSecondStage, 0.0);
	CombatAvionicsSnapshot output = fixture.step(kMissionPeriod * 2);
	const auto& batch = output.external_actions.batch;
	TEST_EXPECT(context, output.fire_control.gun_fire_requested);
	TEST_EXPECT(context, contains(batch, MissionActionKind::GunOn));
	TEST_EXPECT(context, !contains(batch, MissionActionKind::GunOff));
	fixture.step(milliseconds(200)); // Lua has not consumed the request yet.
	fixture.frame.cockpit.mission_adapter.acknowledged_sequence = batch.first_sequence + batch.count - 1;
	output = fixture.step(milliseconds(500)); // Catch-up must treat this as a new observation at 500 ms.
	TEST_EXPECT(context, output.fire_control.gun_fire_requested);
	TEST_EXPECT(context, !contains(output.external_actions.batch, MissionActionKind::GunOff));
	output = fixture.step(milliseconds(578));
	TEST_EXPECT(context, output.fire_control.gun_fire_requested);
	output = fixture.step(milliseconds(610));
	TEST_EXPECT(context, !output.fire_control.gun_fire_requested);
	TEST_EXPECT(context, contains(output.external_actions.batch, MissionActionKind::GunOff));
}

void test_modes_and_immediate_master_cancel(Tests::Context& context)
{
	MissionFixture fixture;
	CombatAvionicsSnapshot output = fixture.step(kMissionPeriod);
	TEST_EXPECT(context, output.mode.master_arm_mode == MasterArmMode::On);
	TEST_EXPECT(context, output.mode.operating_mode == OperatingMode::Navigation);
	fixture.send(CommandId::SetTriggerSecondStage);
	TEST_EXPECT(context, !fixture.step(kMissionPeriod * 2).fire_control.gun_fire_requested);
	fixture.send(CommandId::SelectDogfightMode);
	fixture.send(CommandId::SetTriggerSecondStage);
	fixture.send(CommandId::SetMasterArmSim);
	fixture.send(CommandId::SetMasterArmOn);
	output = fixture.step(kMissionPeriod * 3);
	TEST_EXPECT(context, output.mode.master_arm_mode == MasterArmMode::On);
	TEST_EXPECT(context, output.mode.operating_mode == OperatingMode::Navigation);
	TEST_EXPECT(context, !output.fire_control.gun_fire_requested);
	TEST_EXPECT(context, contains(output.external_actions.batch, MissionActionKind::GunOff));
	TEST_EXPECT(context, contains(output.external_actions.batch, MissionActionKind::AbortCms));
}

void test_commands_wait_for_current_observation_and_retain_order(Tests::Context& context)
{
	MissionFixture fixture;
	fixture.send(CommandId::SelectDogfightMode);
	fixture.send(CommandId::SetTriggerSecondStage);
	fixture.send(CommandId::SetTriggerSecondStage, 0.0);
	const auto output = fixture.step(kMissionPeriod);
	TEST_EXPECT(context, output.fire_control.gun_fire_requested);
	const MissionActionKind expected[] = { MissionActionKind::SelectStation, MissionActionKind::ModeHelmet,
		                                   MissionActionKind::ModeHelmet,    MissionActionKind::LockStart,
		                                   MissionActionKind::ChangeLock,    MissionActionKind::GunOn };
	TEST_EXPECT(context, output.external_actions.batch.count == std::size(expected));
	for (std::size_t i = 0; i < std::size(expected); ++i)
	{
		TEST_EXPECT(context, output.external_actions.batch.actions[i].kind == expected[i]);
	}
}

void test_master_cancel_keeps_action_order_and_legacy_mode_coupling(Tests::Context& context)
{
	MissionFixture fixture;
	fixture.send(CommandId::SelectDogfightMode);
	fixture.send(CommandId::SetTriggerSecondStage);
	auto output = fixture.step(kMissionPeriod);
	const auto selected_revision = output.stores.selection.request_revision;
	fixture.frame.cockpit.mission_adapter.acknowledged_sequence =
	    output.external_actions.batch.first_sequence + output.external_actions.batch.count - 1;
	fixture.step(kMissionPeriod * 2);
	output = fixture.step(kMissionPeriod * 5); // Complete the acknowledged targeting pulse first.
	fixture.frame.cockpit.mission_adapter.acknowledged_sequence =
	    output.external_actions.batch.first_sequence + output.external_actions.batch.count - 1;
	fixture.step(kMissionPeriod * 6);
	fixture.send(CommandId::SetMasterArmOff);
	fixture.send(CommandId::SetMasterArmOn);
	output = fixture.step(kMissionPeriod * 7);
	const MissionActionKind expected[] = { MissionActionKind::GunOff,     MissionActionKind::AbortCms,
		                                   MissionActionKind::LockFinish, MissionActionKind::ChangeLockUp,
		                                   MissionActionKind::ModeCannon, MissionActionKind::SelectStation,
		                                   MissionActionKind::LockFinish, MissionActionKind::ChangeLockUp,
		                                   MissionActionKind::ModeCannon };
	std::vector<MissionActionKind> native_actions;
	for (std::size_t i = 0; i < output.external_actions.batch.count; ++i)
	{
		const auto& action = output.external_actions.batch.actions[i];
		if (action.kind != MissionActionKind::None)
			native_actions.push_back(action.kind);
		if (action.kind == MissionActionKind::SelectStation)
		{
			TEST_EXPECT(context, !action.station);
			TEST_EXPECT(context, action.selection_revision == output.stores.selection.request_revision);
			TEST_EXPECT(context, action.selection_revision > selected_revision);
		}
	}
	TEST_EXPECT(context, native_actions == std::vector<MissionActionKind>(std::begin(expected), std::end(expected)));
	TEST_EXPECT(context, output.stores.selection.pending());
	TEST_EXPECT(context, !output.fire_control.gun_fire_requested);
	TEST_EXPECT(context, output.mode.operating_mode == OperatingMode::Navigation);
	fixture.send(CommandId::SetMasterArmOff);
	fixture.send(CommandId::SelectMissileOverride);
	output = fixture.step(kMissionPeriod * 8);
	TEST_EXPECT(context, output.mode.master_arm_mode == MasterArmMode::On);
	TEST_EXPECT(context, output.mode.operating_mode == OperatingMode::MissileOverride);
}

void test_plain_gun_stop_survives_invalid_inventory_mode_wait(Tests::Context& context)
{
	MissionFixture fixture;
	fixture.send(CommandId::SelectDogfightMode);
	fixture.send(CommandId::SetTriggerSecondStage);
	auto output = fixture.step(kMissionPeriod);
	fixture.frame.cockpit.mission_adapter.acknowledged_sequence =
	    output.external_actions.batch.first_sequence + output.external_actions.batch.count - 1;
	fixture.step(kMissionPeriod * 2);
	fixture.frame.cockpit.weapon_stations.status = { false, 4, ObservationInvalidReason::StationApiError };
	fixture.send(CommandId::SelectDogfightMode);
	fixture.send(CommandId::SetTriggerSecondStage, 0.0);
	bool stopped = false;
	for (int tick = 3; tick <= 40; ++tick)
	{
		output = fixture.step(kMissionPeriod * tick);
		const auto& batch = output.external_actions.batch;
		stopped = stopped || contains(batch, MissionActionKind::GunOff);
		if (batch.count != 0)
			fixture.frame.cockpit.mission_adapter.acknowledged_sequence = batch.first_sequence + batch.count - 1;
	}
	TEST_EXPECT(context, stopped);
	TEST_EXPECT(context, !output.fire_control.gun_fire_requested);
	TEST_EXPECT(context, output.stores.selection.awaiting_inventory);
	fixture.frame.cockpit.weapon_stations.status = { true, 6, ObservationInvalidReason::None };
	output = fixture.step(kMissionPeriod * 41);
	TEST_EXPECT(context, contains(output.external_actions.batch, MissionActionKind::SelectStation));
	TEST_EXPECT(context, !contains(output.external_actions.batch, MissionActionKind::GunOn));
}

void test_inventory_wait_keeps_targeting_release_and_cms_executable(Tests::Context& context)
{
	MissionFixture fixture;
	fixture.frame.cockpit.weapon_stations.status = { false, 4, ObservationInvalidReason::StationApiError };
	fixture.send(CommandId::SelectDogfightMode);
	fixture.send(CommandId::StartCmsForward);
	bool started = false, finished = false, change_released = false, cms_started = false;
	CombatAvionicsSnapshot output;
	for (int tick = 1; tick <= 40; ++tick)
	{
		output = fixture.step(kMissionPeriod * tick);
		const auto& batch = output.external_actions.batch;
		started = started || contains(batch, MissionActionKind::LockStart);
		finished = finished || contains(batch, MissionActionKind::LockFinish);
		change_released = change_released || contains(batch, MissionActionKind::ChangeLockUp);
		cms_started = cms_started || contains(batch, MissionActionKind::StartCmsForward);
		if (batch.count != 0)
			fixture.frame.cockpit.mission_adapter.acknowledged_sequence = batch.first_sequence + batch.count - 1;
	}
	TEST_EXPECT(context, started && finished && change_released);
	TEST_EXPECT(context, cms_started);
	TEST_EXPECT(context, output.stores.selection.awaiting_inventory);
}

void test_radar_on_request_is_not_repeated_for_one_observation(Tests::Context& context)
{
	MissionFixture fixture;
	fixture.frame.cockpit.mission_adapter.radar_power_on = false;
	fixture.send(CommandId::SelectDogfightMode);
	fixture.send(CommandId::SelectMissileOverride);
	fixture.send(CommandId::SetMissileUncage);
	const auto output = fixture.step(kMissionPeriod);
	const auto& batch = output.external_actions.batch;
	const auto count = std::count_if(batch.actions.begin(),
	                                 batch.actions.begin() + batch.count,
	                                 [](const MissionAction& action)
	                                 {
		                                 return action.kind == MissionActionKind::EnsureRadarOn;
	                                 });
	TEST_EXPECT(context, count == 1);
	TEST_EXPECT(context, output.mode.operating_mode == OperatingMode::MissileOverride);
}

void test_pickle_and_targeting_modes(Tests::Context& context)
{
	MissionFixture fixture;
	auto& inventory = fixture.frame.cockpit.weapon_stations;
	inventory.status = { true, 2, ObservationInvalidReason::None };
	for (auto& station : inventory.stations)
		station.configuration = StoreConfiguration::Empty;
	inventory.stations[2] = { StoreConfiguration::Equipped, 2, WeaponCategory::Missile, WeaponRole::AirToAir, "aa" };
	fixture.step(kMissionPeriod);
	fixture.send(CommandId::SelectMissileOverride);
	fixture.send(CommandId::SetWeaponRelease);
	CombatAvionicsSnapshot output = fixture.step(kMissionPeriod * 2);
	TEST_EXPECT(context, !output.fire_control.pickle_requested);
	TEST_EXPECT(context, output.fire_control.air_to_air_submode == AirToAirSubmode::BeyondVisualRange);
	fixture.send(CommandId::SetMissileUncage);
	fixture.send(CommandId::SetWeaponRelease);
	fixture.send(CommandId::SetWeaponRelease, 0.0);
	output = fixture.step(kMissionPeriod * 3);
	TEST_EXPECT(context, output.fire_control.pickle_requested);
	TEST_EXPECT(context, contains(output.external_actions.batch, MissionActionKind::LaunchPermissionOverride));
	fixture.frame.cockpit.mission_adapter.acknowledged_sequence =
	    output.external_actions.batch.first_sequence + output.external_actions.batch.count - 1;
	fixture.step(kMissionPeriod * 4);
	output = fixture.step(kMissionPeriod * 10);
	TEST_EXPECT(context, !output.fire_control.pickle_requested);
	TEST_EXPECT(context, output.stores.air_to_air_missile_count == 2);
	TEST_EXPECT(context, contains(output.external_actions.batch, MissionActionKind::PickleOff));
	TEST_EXPECT(context, output.external_actions.batch.actions[0].kind == MissionActionKind::PickleOff);
	TEST_EXPECT(context, output.external_actions.batch.actions[1].kind == MissionActionKind::LockFinish);
	TEST_EXPECT(context, output.external_actions.batch.actions[2].kind == MissionActionKind::ChangeLockUp);
	inventory.stations[2].count = 1;
	inventory.status.revision = 4;
	output = fixture.step(kMissionPeriod * 11);
	TEST_EXPECT(context, output.stores.air_to_air_missile_count == 1);
}
void test_inventory_authority_selection_and_rearm(Tests::Context& context)
{
	MissionFixture fixture;
	auto& inventory = fixture.frame.cockpit.weapon_stations;
	inventory.status = { true, 2, ObservationInvalidReason::None };
	for (auto& station : inventory.stations)
		station.configuration = StoreConfiguration::Empty;
	inventory.stations[0] = { StoreConfiguration::Equipped, 4, WeaponCategory::Other, WeaponRole::Other, "bomb" };
	inventory.stations[2] = { StoreConfiguration::Equipped,
		                      2,
		                      WeaponCategory::Missile,
		                      WeaponRole::AirToAir,
		                      "opaque-id" };
	inventory.stations[5] = { StoreConfiguration::Equipped,
		                      1,
		                      WeaponCategory::Missile,
		                      WeaponRole::AirToAir,
		                      "{CATM-9}" };
	auto output = fixture.step(kMissionPeriod);
	TEST_EXPECT(context, output.stores.air_to_air_missile_count == 3);
	TEST_EXPECT(context, output.stores.inventory.stations.size() == 9);
	TEST_EXPECT(context, output.stores.inventory.stations[2].count == 2);
	TEST_EXPECT(context, output.stores.inventory.stations[2].weapon_identifier == "opaque-id");
	TEST_EXPECT(context, output.stores.air_to_air_missile_count_complete);
	TEST_EXPECT(context, !output.stores.selection.requested_station);
	fixture.send(CommandId::SelectMissileOverride);
	output = fixture.step(kMissionPeriod * 2);
	TEST_EXPECT(context, output.stores.selection.requested_station == 2);
	TEST_EXPECT(context, output.stores.selection.request_revision == 1);
	TEST_EXPECT(context, !output.stores.selection.acknowledged_station);
	inventory.selected_station = 2;
	inventory.selection_ack_revision = 20; // A future ACK is not confirmation.
	output = fixture.step(kMissionPeriod * 3);
	TEST_EXPECT(context, !output.stores.selection.acknowledged_station);
	inventory.selection_ack_revision = 1;
	output = fixture.step(kMissionPeriod * 4);
	TEST_EXPECT(context, output.stores.selection.acknowledged_station == 2);
	TEST_EXPECT(context, output.stores.selection.request_revision == 1);
	inventory.status.revision = 4;
	inventory.rearm_revision = 1;
	output = fixture.step(kMissionPeriod * 5);
	TEST_EXPECT(context, output.stores.selection.request_revision == 2);
	output = fixture.step(kMissionPeriod * 6);
	TEST_EXPECT(context, output.stores.selection.request_revision == 2);
	inventory.status.revision = 6;
	inventory.stations[2].count = 0;
	output = fixture.step(kMissionPeriod * 7);
	TEST_EXPECT(context, output.stores.air_to_air_missile_count == 1);
	TEST_EXPECT(context, output.stores.selection.requested_station == 5);
	TEST_EXPECT(context, output.stores.selection.request_revision == 3);
	inventory.status.available = false;
	output = fixture.step(kMissionPeriod * 8);
	TEST_EXPECT(context, output.stores.air_to_air_missile_count == 0);
	TEST_EXPECT(context, !output.stores.air_to_air_missile_count_complete);
	TEST_EXPECT(context, output.stores.selection.requested_station == 5);
	inventory.status.invalid_reason = ObservationInvalidReason::StationApiError;
	output = fixture.step(kMissionPeriod * 9);
	TEST_EXPECT(context, output.stores.air_to_air_missile_count == 0);
	TEST_EXPECT(context, !output.stores.air_to_air_missile_count_complete);
	inventory.status = { true, 8, ObservationInvalidReason::None };
	inventory.stations[5].count = 0;
	output = fixture.step(kMissionPeriod * 10);
	TEST_EXPECT(context, output.stores.air_to_air_missile_count == 0);
	TEST_EXPECT(context, output.stores.air_to_air_missile_count_complete);
	TEST_EXPECT(context, !output.stores.selection.requested_station);
	inventory.status.revision = 10;
	inventory.stations[2].count = 1;
	fixture.send(CommandId::SelectDogfightMode);
	output = fixture.step(kMissionPeriod * 11);
	TEST_EXPECT(context, output.stores.selection.requested_station == 2);
	const auto revision = output.stores.selection.request_revision;
	fixture.send(CommandId::SelectMissileOverride);
	output = fixture.step(kMissionPeriod * 12);
	TEST_EXPECT(context, output.stores.selection.request_revision == revision + 1);
	fixture.send(CommandId::SelectNavigationMode);
	output = fixture.step(kMissionPeriod * 13);
	TEST_EXPECT(context, !output.stores.selection.requested_station);
	TEST_EXPECT(context, output.stores.selection.request_revision == revision + 2);
}

void test_seeker_observation_precedence_and_availability(Tests::Context& context)
{
	MissionInputs input;
	ModeData mode;
	StoresData stores;
	TargetingData targeting;
	InfraredMissileData aim9;
	mode.operating_mode = OperatingMode::MissileOverride;
	stores.inventory.stations[0].observation.available = true;
	stores.inventory.stations[0].configuration = StoreConfiguration::Equipped;
	stores.inventory.stations[0].category = WeaponCategory::Missile;
	stores.inventory.stations[0].role = WeaponRole::AirToAir;
	stores.inventory.stations[0].count = 1;
	auto& ir = input.cockpit.ir_seeker;
	auto& radar = input.cockpit.radar;
	ir.status.available = true;
	radar.status.available = true;
	Internal::InfraredMissileSupport processor(input, mode, stores, targeting, aim9);
	ir.locked = true;
	processor.step({}, 0.015625);
	TEST_EXPECT(context, aim9.active && aim9.contact && aim9.locked);
	TEST_EXPECT(context, aim9.readiness == InfraredMissileReadiness::Cool);
	TEST_EXPECT(context, aim9.seeker_state == InfraredSeekerState::Track);
	TEST_EXPECT(context, aim9.requested_tone == InfraredSeekerTone::Lock);
	ir.locked = false;
	ir.target_azimuth_rad = 0.01;
	radar.stt_range_m = 2000.0;
	processor.step({}, 0.015625);
	TEST_EXPECT(context, aim9.contact && !aim9.locked); // Direct IR signal wins over STT range.
	ir.target_azimuth_rad = 0.0;
	processor.step({}, 0.015625);
	TEST_EXPECT(context, !aim9.contact && !aim9.locked); // Caged radar telemetry cannot lock.
	targeting.uncage_held = true;
	ir.desired_azimuth_rad = 0.1;
	ir.desired_elevation_rad = -0.2;
	processor.step({}, 0.015625);
	TEST_EXPECT(context, aim9.contact && aim9.locked);
	TEST_EXPECT_NEAR(context, aim9.seeker_azimuth_rad, 0.1, 1e-12);
	TEST_EXPECT_NEAR(context, aim9.seeker_elevation_rad, -0.2, 1e-12);
	TEST_EXPECT_NEAR(context, aim9.lock_range_m, 2000.0, 1e-12);
	ir.desired_azimuth_rad = 0.0;
	ir.desired_elevation_rad = 0.0;
	radar.stt_range_m = 0.0;
	targeting.designation_requested = true;
	processor.step({}, 0.015625);
	TEST_EXPECT(context, aim9.contact && aim9.locked); // Valid all-zero prototype fallback.
	radar.tdc_azimuth_rad = 0.1;
	processor.step({}, 0.015625);
	TEST_EXPECT(context, !aim9.contact && !aim9.locked); // Real telemetry suppresses that fallback.
	radar.tdc_azimuth_rad = 0.0;
	radar.status.available = false;
	processor.step({}, 0.015625);
	TEST_EXPECT(context, !aim9.contact && !aim9.locked);
	TEST_EXPECT(context,
	            aim9.readiness == InfraredMissileReadiness::Track); // Command readiness is distinct from observation.
	ir.locked = true;
	processor.step({}, 0.015625);
	TEST_EXPECT(context, aim9.locked); // Direct valid IR does not require radar availability.
	ir.status.available = false;
	radar.status.available = true;
	radar.stt_range_m = 2000.0;
	processor.step({}, 0.015625);
	TEST_EXPECT(context, !aim9.contact && !aim9.locked);
	stores.inventory.stations[0].count = 0;
	processor.step({}, 0.015625);
	TEST_EXPECT(context, !aim9.active && aim9.readiness == InfraredMissileReadiness::Off);
	TEST_EXPECT(context,
	            aim9.seeker_state == InfraredSeekerState::Off && aim9.requested_tone == InfraredSeekerTone::Off);
}

void test_control_observations_preserve_fifo_pulses_and_rejected_requests(Tests::Context& context)
{
	MissionFixture fixture;
	auto output = fixture.step(kMissionPeriod);
	TEST_EXPECT(context, output.control_inputs.navigation_press_sequence == 0);
	TEST_EXPECT(context, !output.control_inputs.trigger_second_stage_held);
	fixture.send(CommandId::SelectNavigationMode);
	fixture.send(CommandId::SetMissileUncage);
	fixture.send(CommandId::SetMissileUncage, 0.0);
	fixture.send(CommandId::SetWeaponRelease); // Rejected in NAV, still an observed press.
	fixture.send(CommandId::SetWeaponRelease, 0.0);
	fixture.send(CommandId::SetTriggerSecondStage);
	output = fixture.step(kMissionPeriod * 2);
	TEST_EXPECT(context, output.control_inputs.navigation_press_sequence == 1);
	TEST_EXPECT(context, output.control_inputs.missile_uncage_press_sequence == 2);
	TEST_EXPECT(context, output.control_inputs.weapon_release_press_sequence == 4);
	TEST_EXPECT(context, output.control_inputs.trigger_second_stage_held);
	TEST_EXPECT(context, !output.fire_control.gun_fire_requested && !output.fire_control.pickle_requested);
	TEST_EXPECT(context, output.mode.operating_mode == OperatingMode::Navigation);
	fixture.send(CommandId::SetTriggerSecondStage, 0.0);
	fixture.send(CommandId::SetMasterArmOff);
	fixture.send(CommandId::SelectMissileOverride);
	fixture.send(CommandId::SelectDogfightMode);
	fixture.send(CommandId::SelectNavigationMode);
	output = fixture.step(kMissionPeriod * 3);
	TEST_EXPECT(context, !output.control_inputs.trigger_second_stage_held);
	TEST_EXPECT(context, output.control_inputs.master_arm_press_sequence == 8);
	TEST_EXPECT(context, output.control_inputs.missile_override_press_sequence == 9);
	TEST_EXPECT(context, output.control_inputs.dogfight_press_sequence == 10);
	TEST_EXPECT(context, output.control_inputs.navigation_press_sequence == 11);
	fixture.send(CommandId::SelectNavigationMode, 0.0);
	output = fixture.step(kMissionPeriod * 4);
	TEST_EXPECT(context, output.control_inputs.navigation_press_sequence == 11);
	TEST_EXPECT(context, output.control_inputs.missile_uncage_press_sequence == 2);
	output = fixture.step(kMissionPeriod * 5);
	TEST_EXPECT(context, output.control_inputs.weapon_release_press_sequence == 4);
}

void test_fire_control_publishes_infrared_support_in_the_same_tick(Tests::Context& context)
{
	MissionFixture fixture;
	auto& inventory = fixture.frame.cockpit.weapon_stations;
	inventory.status = { true, 2, ObservationInvalidReason::None };
	inventory.stations[2] = { StoreConfiguration::Equipped, 1, WeaponCategory::Missile, WeaponRole::AirToAir, "aa" };
	fixture.frame.cockpit.ir_seeker.status.available = true;
	fixture.frame.cockpit.ir_seeker.locked = true;
	fixture.send(CommandId::SelectMissileOverride);
	auto output = fixture.step(kMissionPeriod);
	TEST_EXPECT(context, output.infrared_missile.status.available);
	TEST_EXPECT(context, output.infrared_missile.active && output.infrared_missile.locked);
	TEST_EXPECT(context, output.infrared_missile.requested_tone == InfraredSeekerTone::Lock);
	fixture.send(CommandId::SelectNavigationMode);
	output = fixture.step(kMissionPeriod * 2);
	TEST_EXPECT(context, !output.infrared_missile.active && !output.infrared_missile.locked);
	TEST_EXPECT(context, output.infrared_missile.requested_tone == InfraredSeekerTone::Off);
}

void test_gun_quantity_observations_and_catch_up(Tests::Context& context)
{
	MissionFixture fixture;
	auto output = fixture.step(kMissionPeriod);
	TEST_EXPECT(context, !output.stores.gun_ammunition.count);
	TEST_EXPECT(context, !output.stores.gun_ammunition.observation.available);
	fixture.frame.cockpit.gun_ammunition = { { true, 2, ObservationInvalidReason::None }, 261 };
	fixture.send(CommandId::SelectDogfightMode);
	fixture.send(CommandId::SetTriggerSecondStage);
	output = fixture.step(seconds(4));
	TEST_EXPECT(context, output.fire_control.gun_fire_requested);
	TEST_EXPECT_NEAR(context, output.stores.gun_count, 261.0, 1e-12);
	TEST_EXPECT(context, output.stores.gun_ammunition.observation.available);
	output = fixture.step(milliseconds(4100));
	TEST_EXPECT_NEAR(context, output.stores.gun_count, 261.0, 1e-12);
	fixture.frame.cockpit.gun_ammunition = { { true, 4, ObservationInvalidReason::None }, 216 };
	output = fixture.step(milliseconds(4200));
	TEST_EXPECT_NEAR(context, output.stores.gun_count, 216.0, 1e-12);
	fixture.frame.cockpit.gun_ammunition = { { true, 6, ObservationInvalidReason::None }, 0 };
	output = fixture.step(milliseconds(4300));
	TEST_EXPECT(context, output.stores.gun_ammunition.observation.available);
	TEST_EXPECT_NEAR(context, output.stores.gun_count, 0.0, 1e-12);
	fixture.frame.cockpit.gun_ammunition.status.available = false;
	output = fixture.step(milliseconds(4400));
	TEST_EXPECT(context, !output.stores.gun_ammunition.observation.available);
	TEST_EXPECT_NEAR(context, output.stores.gun_count, 0.0, 1e-12);
}

void test_gun_restart_in_one_fifo_does_not_backdate_quantity(Tests::Context& context)
{
	MissionFixture fixture;
	fixture.frame.cockpit.gun_ammunition = { { true, 2, ObservationInvalidReason::None }, 216 };
	fixture.send(CommandId::SelectDogfightMode);
	fixture.send(CommandId::SetTriggerSecondStage);
	fixture.step(kMissionPeriod);
	fixture.send(CommandId::SetMasterArmOff);
	fixture.send(CommandId::SelectDogfightMode);
	fixture.send(CommandId::SetTriggerSecondStage);
	const auto output = fixture.step(seconds(4));
	TEST_EXPECT_NEAR(context, output.stores.gun_count, 216.0, 1e-12);
	TEST_EXPECT(context, !output.stores.gun_ammunition.observation.available);
}

void test_cms_commands_share_fifo_with_mode_commands(Tests::Context& context)
{
	MissionFixture fixture;
	fixture.send(CommandId::StartCmsForward);
	fixture.send(CommandId::SetMasterArmOff);
	fixture.send(CommandId::StartCmsAft); // OFF rejects start.
	fixture.send(CommandId::SetMasterArmSim);
	fixture.send(CommandId::StartCmsLeft); // SIM rejects start.
	fixture.send(CommandId::StartCmsRight); // Abort is always allowed.
	fixture.send(CommandId::SetMasterArmOn);
	fixture.send(CommandId::StartCmsAft);
	fixture.send(CommandId::StartCmsLeft);
	fixture.send(CommandId::StartCmsForward, 0.0); // Releases do not start a program.
	fixture.send(CommandId::PressCms); // Reserved.
	const auto output = fixture.step(kMissionPeriod);
	std::vector<MissionActionKind> cms;
	auto window = output.external_actions.batch;
	int tick = 2;
	while (window.count != 0)
	{
		for (std::size_t i = 0; i < window.count; ++i)
		{
			const auto action = window.actions[i].kind;
			if (action == MissionActionKind::AbortCms || action == MissionActionKind::StartCmsForward ||
			    action == MissionActionKind::StartCmsAft || action == MissionActionKind::StartCmsLeft)
				cms.push_back(action);
		}
		fixture.frame.cockpit.mission_adapter.acknowledged_sequence = window.first_sequence + window.count - 1;
		window = fixture.step(kMissionPeriod * tick++).external_actions.batch;
	}
	const std::vector<MissionActionKind> expected = {
		MissionActionKind::StartCmsForward, MissionActionKind::AbortCms,    MissionActionKind::AbortCms,
		MissionActionKind::AbortCms,        MissionActionKind::StartCmsAft, MissionActionKind::StartCmsLeft
	};
	TEST_EXPECT(context, cms == expected);
	TEST_EXPECT(context, output.mode.master_arm_mode == MasterArmMode::On);
	TEST_EXPECT(context, output.mode.operating_mode == OperatingMode::Navigation);
	TEST_EXPECT(context, output.mode.status.available);
	TEST_EXPECT(context, output.mode.status.revision == output.status.revision);
}

void test_repeat_mode_selection_still_cycles_weapon(Tests::Context& context)
{
	MissionFixture fixture;
	auto& inventory = fixture.frame.cockpit.weapon_stations;
	inventory.status = { true, 2, ObservationInvalidReason::None };
	for (auto& station : inventory.stations)
		station.configuration = StoreConfiguration::Empty;
	for (int i : { 2, 8 })
		inventory.stations[i] = { StoreConfiguration::Equipped,
			                      1,
			                      WeaponCategory::Missile,
			                      WeaponRole::AirToAir,
			                      "aa" };
	fixture.send(CommandId::SelectMissileOverride);
	fixture.send(CommandId::SelectMissileOverride);
	const auto output = fixture.step(kMissionPeriod);
	TEST_EXPECT(context, output.stores.selection.requested_station == 8);
	TEST_EXPECT(context, output.mode.operating_mode == OperatingMode::MissileOverride);
}

void test_mc_subscriptions_are_isolated_between_instances(Tests::Context& context)
{
	MissionFixture first;
	MissionFixture second;
	first.send(CommandId::SelectDogfightMode);
	first.send(CommandId::SetTriggerSecondStage);
	const auto armed = first.step(kMissionPeriod);
	second.send(CommandId::SetMasterArmOff);
	const auto safe = second.step(kMissionPeriod);
	TEST_EXPECT(context, armed.fire_control.gun_fire_requested);
	TEST_EXPECT(context, safe.mode.master_arm_mode == MasterArmMode::Off);
	TEST_EXPECT(context, !safe.fire_control.gun_fire_requested);
	TEST_EXPECT(context, first.step(kMissionPeriod * 2).fire_control.gun_fire_requested);
}

}

void run_mission_computer_tests(Tests::Context& context)
{
	test_startup_and_schedule(context);
	test_default_assembly(context);
	test_entry_in_pipeline(context);
	test_action_delivery_retains_order_and_rejects_future_ack(context);
	test_short_trigger_waits_for_execution_and_does_not_backdate_ack(context);
	test_modes_and_immediate_master_cancel(context);
	test_commands_wait_for_current_observation_and_retain_order(context);
	test_master_cancel_keeps_action_order_and_legacy_mode_coupling(context);
	test_inventory_wait_keeps_targeting_release_and_cms_executable(context);
	test_plain_gun_stop_survives_invalid_inventory_mode_wait(context);
	test_radar_on_request_is_not_repeated_for_one_observation(context);
	test_pickle_and_targeting_modes(context);
	test_inventory_authority_selection_and_rearm(context);
	test_seeker_observation_precedence_and_availability(context);
	test_control_observations_preserve_fifo_pulses_and_rejected_requests(context);
	test_fire_control_publishes_infrared_support_in_the_same_tick(context);
	test_gun_quantity_observations_and_catch_up(context);
	test_gun_restart_in_one_fifo_does_not_backdate_quantity(context);
	test_cms_commands_share_fifo_with_mode_commands(context);
	test_repeat_mode_selection_still_cycles_weapon(context);
	test_mc_subscriptions_are_isolated_between_instances(context);
}
