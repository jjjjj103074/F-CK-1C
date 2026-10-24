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

static_assert(!std::is_copy_constructible<MissionComputer>::value,
              "MC must preserve database and task target addresses.");
static_assert(!std::is_move_constructible<MissionComputer>::value,
              "Moving MC would invalidate constructor-bound references.");

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
	TEST_EXPECT(context, tables.tasks()[0].state.completed_ticks == 64);
	TEST_EXPECT(context, tables.tasks()[1].state.completed_ticks == 64);
}

void test_entry_in_pipeline(Tests::Context& context)
{
	// MC uses the built-in FrameInput and publishes a typed snapshot without other Systems.
	SystemEntry entry = Catalog::MissionComputer::create_entry();
	TEST_EXPECT(context, entry.id == "mission_computer");
	SystemPipeline pipeline(SystemPipelineTest::flight_setup(), { entry });
	TEST_EXPECT(context, pipeline.system_count() == 1);
	const AircraftDataSnapshot snapshot = SystemPipelineTest::step_pipeline_to(pipeline, {}, {}, seconds(1));
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
	fixture.step(milliseconds(16));
	fixture.send(CommandId::SelectDogfightMode);
	fixture.send(CommandId::SetTriggerSecondStage);
	fixture.send(CommandId::SetTriggerSecondStage, 0.0);
	CombatAvionicsSnapshot output = fixture.step(milliseconds(32));
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
	CombatAvionicsSnapshot output = fixture.step(milliseconds(16));
	TEST_EXPECT(context, output.fire_control.master_arm_mode == MasterArmMode::On);
	TEST_EXPECT(context, output.fire_control.fire_control_mode == FireControlMode::Navigation);
	fixture.send(CommandId::SetTriggerSecondStage);
	TEST_EXPECT(context, !fixture.step(milliseconds(32)).fire_control.gun_fire_requested);
	fixture.send(CommandId::SelectDogfightMode);
	fixture.send(CommandId::SetTriggerSecondStage);
	fixture.send(CommandId::SetMasterArmSim);
	fixture.send(CommandId::SetMasterArmOn);
	output = fixture.step(milliseconds(48));
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
	const auto output = fixture.step(milliseconds(16));
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

void test_radar_on_request_is_not_repeated_for_one_observation(Tests::Context& context)
{
	MissionFixture fixture;
	fixture.frame.cockpit.mission_adapter.radar_power_on = false;
	fixture.send(CommandId::SelectDogfightMode);
	fixture.send(CommandId::SelectMissileOverride);
	fixture.send(CommandId::SetMissileUncage);
	const auto output = fixture.step(milliseconds(16));
	const auto& batch = output.external_actions.batch;
	const auto count =
	    std::count(batch.actions.begin(), batch.actions.begin() + batch.count, MissionActionKind::EnsureRadarOn);
	TEST_EXPECT(context, count == 1);
	TEST_EXPECT(context, output.fire_control.fire_control_mode == FireControlMode::MissileOverride);
}

void test_pickle_and_sensor_modes(Tests::Context& context)
{
	MissionFixture fixture;
	fixture.step(milliseconds(16));
	fixture.send(CommandId::SelectMissileOverride);
	fixture.send(CommandId::SetWeaponRelease);
	CombatAvionicsSnapshot output = fixture.step(milliseconds(32));
	TEST_EXPECT(context, !output.fire_control.pickle_requested);
	TEST_EXPECT(context, output.fire_control.air_to_air_submode == AirToAirSubmode::BeyondVisualRange);
	fixture.send(CommandId::SetMissileUncage);
	fixture.send(CommandId::SetWeaponRelease);
	fixture.send(CommandId::SetWeaponRelease, 0.0);
	output = fixture.step(milliseconds(48));
	TEST_EXPECT(context, output.fire_control.pickle_requested);
	TEST_EXPECT(context, contains(output.external_actions.batch, MissionActionKind::LaunchPermissionOverride));
	fixture.frame.cockpit.mission_adapter.acknowledged_sequence =
	    output.external_actions.batch.first_sequence + output.external_actions.batch.count - 1;
	fixture.step(milliseconds(64));
	output = fixture.step(milliseconds(160));
	TEST_EXPECT(context, !output.fire_control.pickle_requested);
	TEST_EXPECT(context, output.stores.aim9_count == 0);
	TEST_EXPECT(context, contains(output.external_actions.batch, MissionActionKind::PickleOff));
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
	test_radar_on_request_is_not_repeated_for_one_observation(context);
	test_pickle_and_sensor_modes(context);
}
