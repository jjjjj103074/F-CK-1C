#include "SystemPipelineTestFixture.h"

#include "Core/Systems/MissionComputer/Components/Executive/Executive.h"
#include "Core/Systems/MissionComputer/Components/StartupAndRestart/StartupAndRestart.h"
#include "Core/Systems/MissionComputer/MissionComputer.h"

#include <chrono>
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
	// 實際 Entry 必須能獨立接入 Pipeline，且不要求尚未定義的任務輸入。
	SystemEntry entry = Catalog::MissionComputer::create_entry();
	TEST_EXPECT(context, entry.id == "mission_computer");
	SystemPipeline pipeline(SystemPipelineTest::flight_setup(), { entry });
	TEST_EXPECT(context, pipeline.system_count() == 1);
	const AircraftDataSnapshot snapshot = SystemPipelineTest::step_pipeline_to(pipeline, {}, {}, seconds(1));
	TEST_EXPECT(context, pipeline.advanced_through() == seconds(1));
	TEST_EXPECT(context, !snapshot.has(AircraftDataKeys::kPilotControlSignal));
	TEST_EXPECT(context, !snapshot.has(AircraftDataKeys::kFuelData));
}
}

void run_mission_computer_tests(Tests::Context& context)
{
	test_startup_and_schedule(context);
	test_default_assembly(context);
	test_entry_in_pipeline(context);
}
