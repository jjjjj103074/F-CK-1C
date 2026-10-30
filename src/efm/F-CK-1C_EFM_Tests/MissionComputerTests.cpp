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
	const auto before = SystemPipelineTest::step_pipeline_to(pipeline, {}, {}, kMissionPeriod - nanoseconds(1));
	TEST_EXPECT(context, !before.read(AircraftDataKeys::kMissionComputerSnapshot).status.available);
	const auto first = SystemPipelineTest::step_pipeline_to(pipeline, {}, {}, kMissionPeriod);
	TEST_EXPECT(context, first.read(AircraftDataKeys::kMissionComputerSnapshot).status.revision == 1);
	const AircraftDataSnapshot snapshot = SystemPipelineTest::step_pipeline_to(pipeline, {}, {}, seconds(1));
	TEST_EXPECT(context, snapshot.read(AircraftDataKeys::kMissionComputerSnapshot).status.revision == 32);
	TEST_EXPECT(context, pipeline.advanced_through() == seconds(1));
	TEST_EXPECT(context, !snapshot.has(AircraftDataKeys::kPilotControlSignal));
	TEST_EXPECT(context, !snapshot.has(AircraftDataKeys::kFuelData));
	TEST_EXPECT(context, snapshot.has(AircraftDataKeys::kMissionComputerSnapshot));
	TEST_EXPECT(context, snapshot.read(AircraftDataKeys::kMissionComputerSnapshot).status.available);
}

struct MissionFixture
{
	SystemPipeline pipeline{ SystemPipelineTest::flight_setup(), { Catalog::MissionComputer::create_entry() } };
	FrameInput frame;

	MissionFixture()
	{
		frame.cockpit.mission_adapter.available = true;
		frame.cockpit.mission_adapter.radar_power_on = true;
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
	return std::find(batch.actions.begin(), batch.actions.begin() + batch.count, action) !=
	       batch.actions.begin() + batch.count;
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
		TEST_EXPECT(context, batch.actions[i] == (i % 2 == 0 ? MissionActionKind::GunOn : MissionActionKind::GunOff));
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
	TEST_EXPECT(context, output.fire_control.master_arm_mode == MasterArmMode::On);
	TEST_EXPECT(context, output.fire_control.fire_control_mode == FireControlMode::Navigation);
	fixture.send(CommandId::SetTriggerSecondStage);
	TEST_EXPECT(context, !fixture.step(kMissionPeriod * 2).fire_control.gun_fire_requested);
	fixture.send(CommandId::SelectDogfightMode);
	fixture.send(CommandId::SetTriggerSecondStage);
	fixture.send(CommandId::SetMasterArmSim);
	fixture.send(CommandId::SetMasterArmOn);
	output = fixture.step(kMissionPeriod * 3);
	TEST_EXPECT(context, output.fire_control.master_arm_mode == MasterArmMode::On);
	TEST_EXPECT(context, output.fire_control.fire_control_mode == FireControlMode::Navigation);
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
	const MissionActionKind expected[] = { MissionActionKind::CycleWeapon, MissionActionKind::ModeHelmet,
		                                   MissionActionKind::ModeHelmet,  MissionActionKind::LockStart,
		                                   MissionActionKind::ChangeLock,  MissionActionKind::GunOn };
	TEST_EXPECT(context, output.external_actions.batch.count == std::size(expected));
	for (std::size_t i = 0; i < std::size(expected); ++i)
	{
		TEST_EXPECT(context, output.external_actions.batch.actions[i] == expected[i]);
	}
}

void test_master_cancel_keeps_action_order_and_legacy_mode_coupling(Tests::Context& context)
{
	MissionFixture fixture;
	fixture.send(CommandId::SelectDogfightMode);
	fixture.send(CommandId::SetTriggerSecondStage);
	auto output = fixture.step(kMissionPeriod);
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
	const MissionActionKind expected[] = { MissionActionKind::GunOff,       MissionActionKind::AbortCms,
		                                   MissionActionKind::LockFinish,   MissionActionKind::ChangeLockUp,
		                                   MissionActionKind::ModeCannon,   MissionActionKind::LockFinish,
		                                   MissionActionKind::ChangeLockUp, MissionActionKind::ModeCannon };
	TEST_EXPECT(context, output.external_actions.batch.count == std::size(expected));
	if (output.external_actions.batch.count == std::size(expected))
	{
		for (std::size_t i = 0; i < std::size(expected); ++i)
			TEST_EXPECT(context, output.external_actions.batch.actions[i] == expected[i]);
	}
	TEST_EXPECT(context, !output.fire_control.gun_fire_requested);
	TEST_EXPECT(context, output.fire_control.fire_control_mode == FireControlMode::Navigation);
	fixture.send(CommandId::SetMasterArmOff);
	fixture.send(CommandId::SelectMissileOverride);
	output = fixture.step(kMissionPeriod * 8);
	TEST_EXPECT(context, output.fire_control.master_arm_mode == MasterArmMode::On);
	TEST_EXPECT(context, output.fire_control.fire_control_mode == FireControlMode::MissileOverride);
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
	const auto count =
	    std::count(batch.actions.begin(), batch.actions.begin() + batch.count, MissionActionKind::EnsureRadarOn);
	TEST_EXPECT(context, count == 1);
	TEST_EXPECT(context, output.fire_control.fire_control_mode == FireControlMode::MissileOverride);
}

void test_pickle_and_targeting_modes(Tests::Context& context)
{
	MissionFixture fixture;
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
	TEST_EXPECT(context, output.stores.air_to_air_missile_count == 0);
	TEST_EXPECT(context, contains(output.external_actions.batch, MissionActionKind::PickleOff));
}
void test_inventory_authority_selection_and_rearm(Tests::Context& context)
{
	MissionFixture fixture;
	auto& inventory = fixture.frame.cockpit.weapon_stations;
	inventory.status = { true, 2, ObservationInvalidReason::None };
	inventory.stations[0] = { true, 4, WeaponCategory::Other, WeaponRole::Other, "bomb" };
	inventory.stations[2] = { true, 2, WeaponCategory::Missile, WeaponRole::AirToAir, "opaque-id" };
	inventory.stations[5] = { true, 1, WeaponCategory::Other, WeaponRole::Other, "{CATM-9}" };
	fixture.frame.cockpit.mission_adapter.missile_count_available = true;
	fixture.frame.cockpit.mission_adapter.missile_count = 9;
	auto output = fixture.step(kMissionPeriod);
	TEST_EXPECT(context, output.stores.air_to_air_missile_count == 3);
	TEST_EXPECT(context, output.stores.count_source == StoresCountSource::StationInventory);
	TEST_EXPECT(context, !output.stores.requested_station);
	fixture.send(CommandId::SelectMissileOverride);
	output = fixture.step(kMissionPeriod * 2);
	TEST_EXPECT(context, output.stores.requested_station == 2);
	TEST_EXPECT(context, output.stores.selection_request_revision == 1);
	TEST_EXPECT(context, !output.stores.selected_station);
	inventory.selected_station = 2;
	inventory.selection_ack_revision = 20; // A future ACK is not confirmation.
	output = fixture.step(kMissionPeriod * 3);
	TEST_EXPECT(context, !output.stores.selected_station);
	inventory.selection_ack_revision = 1;
	output = fixture.step(kMissionPeriod * 4);
	TEST_EXPECT(context, output.stores.selected_station == 2);
	TEST_EXPECT(context, output.stores.selection_request_revision == 1);
	inventory.rearm_revision = 1;
	output = fixture.step(kMissionPeriod * 5);
	TEST_EXPECT(context, output.stores.selection_request_revision == 2);
	output = fixture.step(kMissionPeriod * 6);
	TEST_EXPECT(context, output.stores.selection_request_revision == 2);
	inventory.stations[2].count = 0;
	output = fixture.step(kMissionPeriod * 7);
	TEST_EXPECT(context, output.stores.air_to_air_missile_count == 1);
	TEST_EXPECT(context, output.stores.requested_station == 5);
	TEST_EXPECT(context, output.stores.selection_request_revision == 3);
	inventory.status.available = false;
	output = fixture.step(kMissionPeriod * 8);
	TEST_EXPECT(context, output.stores.air_to_air_missile_count == 9);
	TEST_EXPECT(context, output.stores.count_source == StoresCountSource::BaseDataApi);
	TEST_EXPECT(context, output.stores.requested_station == 5);
	fixture.frame.cockpit.mission_adapter.missile_count_available = false;
	output = fixture.step(kMissionPeriod * 9);
	TEST_EXPECT(context, output.stores.air_to_air_missile_count == 9);
	TEST_EXPECT(context, output.stores.count_source == StoresCountSource::Estimate);
	inventory.status.available = true;
	inventory.stations[5].count = 0;
	output = fixture.step(kMissionPeriod * 10);
	TEST_EXPECT(context, output.stores.air_to_air_missile_count == 0);
	TEST_EXPECT(context, output.stores.count_source == StoresCountSource::StationInventory);
	TEST_EXPECT(context, !output.stores.requested_station);
	inventory.stations[2].count = 1;
	fixture.send(CommandId::SelectDogfightMode);
	output = fixture.step(kMissionPeriod * 11);
	TEST_EXPECT(context, output.stores.requested_station == 2);
	const auto revision = output.stores.selection_request_revision;
	fixture.send(CommandId::SelectMissileOverride);
	output = fixture.step(kMissionPeriod * 12);
	TEST_EXPECT(context, output.stores.selection_request_revision == revision + 1);
	fixture.send(CommandId::SelectNavigationMode);
	output = fixture.step(kMissionPeriod * 13);
	TEST_EXPECT(context, !output.stores.requested_station);
	TEST_EXPECT(context, output.stores.selection_request_revision == revision + 2);
}

void test_seeker_observation_precedence_and_availability(Tests::Context& context)
{
	MissionInputs input;
	ModeData mode;
	StoresData stores;
	TargetingData targeting;
	InfraredMissileData aim9;
	mode.fire_control = FireControlMode::MissileOverride;
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
	stores.missile_count = 0;
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
	TEST_EXPECT(context, output.fire_control.fire_control_mode == FireControlMode::Navigation);
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
	fixture.step(kMissionPeriod);
	fixture.send(CommandId::SetMasterArmOff);
	fixture.send(CommandId::SetTriggerSecondStage);
	auto output = fixture.step(seconds(4));
	TEST_EXPECT(context, !output.fire_control.gun_fire_requested);
	TEST_EXPECT_NEAR(context, output.stores.gun_count, 523.0, 1e-12);
	fixture.send(CommandId::SetMasterArmOn);
	fixture.send(CommandId::SelectDogfightMode);
	fixture.send(CommandId::SetTriggerSecondStage);
	output = fixture.step(seconds(8));
	TEST_EXPECT(context, output.fire_control.gun_fire_requested);
	TEST_EXPECT_NEAR(context, output.stores.gun_count, 523.0, 1e-12); // Do not backdate a new press by 4 seconds.
	output = fixture.step(milliseconds(8100));
	TEST_EXPECT_NEAR(context, output.stores.gun_count, 513.0, 1e-10);
	fixture.frame.cockpit.mission_adapter.gun_count_available = true;
	fixture.frame.cockpit.mission_adapter.gun_count = 0.0;
	output = fixture.step(milliseconds(8132));
	TEST_EXPECT_NEAR(context, output.stores.gun_count, 0.0, 1e-12);
	fixture.frame.cockpit.mission_adapter.gun_count_available = false;
	output = fixture.step(milliseconds(8164));
	TEST_EXPECT_NEAR(context, output.stores.gun_count, 0.0, 1e-12);
	fixture.send(CommandId::SetMasterArmOff);
	output = fixture.step(milliseconds(8200));
	TEST_EXPECT_NEAR(context, output.stores.gun_count, 0.0, 1e-12);
}

void test_gun_restart_in_one_fifo_does_not_backdate_quantity(Tests::Context& context)
{
	MissionFixture fixture;
	fixture.step(kMissionPeriod);
	fixture.send(CommandId::SelectDogfightMode);
	fixture.send(CommandId::SetTriggerSecondStage);
	auto output = fixture.step(kMissionPeriod * 2);
	TEST_EXPECT_NEAR(context, output.stores.gun_count, 523.0, 1e-12);
	fixture.send(CommandId::SetMasterArmOff);
	fixture.send(CommandId::SelectDogfightMode);
	fixture.send(CommandId::SetTriggerSecondStage);
	output = fixture.step(seconds(4));
	TEST_EXPECT_NEAR(context, output.stores.gun_count, 523.0, 1e-12);
	output = fixture.step(milliseconds(4100));
	TEST_EXPECT_NEAR(context, output.stores.gun_count, 513.0, 1e-10);
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
	test_radar_on_request_is_not_repeated_for_one_observation(context);
	test_pickle_and_targeting_modes(context);
	test_inventory_authority_selection_and_rearm(context);
	test_seeker_observation_precedence_and_availability(context);
	test_control_observations_preserve_fifo_pulses_and_rejected_requests(context);
	test_fire_control_publishes_infrared_support_in_the_same_tick(context);
	test_gun_quantity_observations_and_catch_up(context);
	test_gun_restart_in_one_fifo_does_not_backdate_quantity(context);
}
