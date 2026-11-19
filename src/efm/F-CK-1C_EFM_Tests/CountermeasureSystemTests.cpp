#include "TestHarness.h"
#include "DebugTelemetryTestSupport.h"
#include "DcsAircraftTestFixture.h"
#include "Core/Fck1cEfm.h"
#include "DcsBridge/Internal/Countermeasures/CountermeasureExecutor.h"
#include <string>
#include <vector>
namespace
{
using namespace Core;
using namespace DcsBridge::Internal;
struct Fixture
{
	Tests::Aircraft::Fixture host;
	DcsAircraftInterface aircraft{ host.backend() };
	std::vector<std::string> logs;
	CountermeasureExecutor executor{ aircraft,
		                             [this](const EventRecord& e)
		                             {
		                                 logs.emplace_back(e.message);
		                             } };
	Fck1cEfm core{ Tests::disabled_debug_telemetry() };
	FrameInput input;
	FrameOutput output;
	std::vector<double> execution_times;
	Fixture()
	{
		output = core.start(StartMode::HotAir);
		input.dt_s = 0.03125;
	}
	void send(CommandId id, double value = 1)
	{
		core.handle_command({ id, value });
	}
	void step(int count = 1)
	{
		for (int i = 0; i < count; ++i)
		{
			input.cockpit.countermeasures = executor.observe();
			output = core.step(input);
			const auto before = input.cockpit.countermeasures.execution.sequence;
			executor.execute(output.cockpit.countermeasures, output.simulation_time_s);
			const auto after = executor.observe();
			if (after.execution.sequence != before)
				execution_times.push_back(after.execution.completed_at_s);
		}
	}
};
void test_programs_reach_native_dispensing_and_stock(Tests::Context& c)
{
	for (const auto arm : { CommandId::SetMasterArmOff, CommandId::SetMasterArmSim, CommandId::SetMasterArmOn })
		for (const bool forward : { false, true })
		{
			Fixture f;
			f.send(arm);
			f.send(forward ? CommandId::StartCmsForward : CommandId::StartCmsLeft);
			f.send(forward ? CommandId::StartCmsForward : CommandId::StartCmsLeft, 0);
			f.step(650);
			const auto& cm = f.output.cockpit.countermeasures;
			TEST_EXPECT(c, cm.state == CountermeasureProgramState::Completed);
			TEST_EXPECT(c, cm.accepted_sequence == 1 && cm.completed_rounds == (forward ? 10 : 7));
			TEST_EXPECT(c, cm.remaining_rounds == 0 && !cm.dispense);
			TEST_EXPECT(c, f.host.flare == (forward ? 80 : 90) && f.host.chaff == (forward ? 80 : 83));
			TEST_EXPECT(c, f.host.drops == (forward ? 20 : 7));
			TEST_EXPECT(c, f.output.cockpit.combat_avionics.action_requests.items.empty());
			TEST_EXPECT(c, f.executor.observe().execution.sequence == (forward ? 10 : 7));
			TEST_EXPECT(c, cm.observation.inventory.available && cm.observation.inventory.chaff == f.host.chaff);
			TEST_EXPECT(c, f.execution_times.size() == static_cast<std::size_t>(forward ? 10 : 7));
			for (std::size_t i = 1; i < f.execution_times.size(); ++i)
				TEST_EXPECT_NEAR(c, f.execution_times[i] - f.execution_times[i - 1], forward ? 1.0 : 0.5, 1e-8);
			f.step(100);
			TEST_EXPECT(c, f.host.drops == (forward ? 20 : 7));
		}
}
void test_program_replacement_and_inputs(Tests::Context& c)
{
	Fixture f;
	f.send(CommandId::StartCmsForward);
	f.step(35);
	TEST_EXPECT(c, f.host.flare == 88 && f.host.chaff == 88);
	f.send(CommandId::StartCmsRight);
	f.send(CommandId::StartCmsAft);
	f.send(CommandId::SetMasterArmSim);
	f.step(2);
	TEST_EXPECT(c, f.output.cockpit.countermeasures.accepted_sequence == 1);
	f.send(CommandId::StartCmsLeft);
	f.step(300);
	TEST_EXPECT(c, f.host.flare == 88 && f.host.chaff == 81);
	TEST_EXPECT(c, f.output.cockpit.countermeasures.completed_rounds == 7);
	f.send(CommandId::StartCmsLeft);
	f.send(CommandId::StartCmsForward);
	f.step(650);
	TEST_EXPECT(c, f.host.flare == 78 && f.host.chaff == 71);
	TEST_EXPECT(c, f.output.cockpit.countermeasures.accepted_sequence == 4);
}
void test_slow_frames_wait_for_execution(Tests::Context& c)
{
	Fixture f;
	f.input.dt_s = 1.2;
	f.send(CommandId::StartCmsLeft);
	f.step();
	TEST_EXPECT(c, f.host.drops == 1);
	f.step();
	TEST_EXPECT(c, f.host.drops == 2);
	f.step(8);
	TEST_EXPECT(c, f.host.drops == 7 && f.host.chaff == 83);
	for (std::size_t i = 1; i < f.execution_times.size(); ++i)
		TEST_EXPECT(c, f.execution_times[i] - f.execution_times[i - 1] >= 0.5);
}
void test_native_failures_stop_program_and_reentry_resets(Tests::Context& c)
{
	Fixture f;
	f.host.fail_drop = true;
	f.send(CommandId::StartCmsLeft);
	f.step(200);
	TEST_EXPECT(c, f.host.drops == 1 && f.host.chaff == 90);
	TEST_EXPECT(c, f.output.cockpit.countermeasures.state == CountermeasureProgramState::Failed);
	TEST_EXPECT(c, !f.executor.observe().execution.chaff_returned);
	TEST_EXPECT(c, !f.logs.empty() && f.logs.back().find("returned=0") != std::string::npos);
	f.host.fail_drop = false;
	f.send(CommandId::StartCmsForward);
	f.step(2);
	TEST_EXPECT(c, f.host.flare == 89 && f.host.chaff == 89);
	f.core.release();
	f.executor.reset_flight();
	f.aircraft.reset_flight();
	f.output = f.core.start(StartMode::HotAir);
	f.step(100);
	TEST_EXPECT(c, f.host.flare == 89 && f.host.chaff == 89);
	TEST_EXPECT(c, f.output.cockpit.countermeasures.state == CountermeasureProgramState::Idle);
	f.send(CommandId::StartCmsLeft);
	f.step(200);
	TEST_EXPECT(c, f.host.chaff == 82 && f.host.flare == 89);
	TEST_EXPECT(c, f.output.cockpit.countermeasures.accepted_sequence == 1);
}
void test_returned_call_and_observed_inventory_are_separate(Tests::Context& c)
{
	Tests::Aircraft::Fixture host;
	auto backend = host.backend();
	backend.dispense = [&host](std::uintptr_t, std::uintptr_t, std::uint32_t&)
	{
		++host.drops;
		return true;
	};
	DcsAircraftInterface aircraft(backend);
	CountermeasureExecutor executor(aircraft, {});
	CountermeasureSnapshot request;
	request.status = { true, 1 };
	request.accepted_sequence = 1;
	request.dispense = CountermeasureDispenseRequest{ 1, false, true };
	for (int i = 0; i < 10; ++i)
		executor.execute(request, 1.0);
	const auto observation = executor.observe();
	TEST_EXPECT(c, host.drops == 1);
	TEST_EXPECT(c, observation.execution.sequence == 1 && observation.execution.chaff_returned);
	TEST_EXPECT(c, observation.inventory.available && observation.inventory.chaff == 90);
}
void test_partial_empty_and_unavailable_native_context(Tests::Context& c)
{
	Fixture f;
	f.host.flare = 0;
	f.send(CommandId::StartCmsForward);
	f.step(200);
	TEST_EXPECT(c, f.host.drops == 1 && f.host.flare == 0 && f.host.chaff == 89);
	TEST_EXPECT(c, f.output.cockpit.countermeasures.state == CountermeasureProgramState::Failed);
	f.send(CommandId::StartCmsLeft);
	f.step(200);
	TEST_EXPECT(c, f.host.chaff == 82);
	Fixture absent;
	absent.host.memory[0x1010] = 0;
	absent.send(CommandId::StartCmsLeft);
	absent.step(200);
	TEST_EXPECT(c, absent.host.drops == 0);
	TEST_EXPECT(c, !absent.output.cockpit.countermeasures.observation.inventory.available);
	TEST_EXPECT(c, absent.output.cockpit.countermeasures.state == CountermeasureProgramState::Failed);
}
}
void run_countermeasure_system_tests(Tests::Context& c)
{
	test_programs_reach_native_dispensing_and_stock(c);
	test_returned_call_and_observed_inventory_are_separate(c);
	test_program_replacement_and_inputs(c);
	test_slow_frames_wait_for_execution(c);
	test_native_failures_stop_program_and_reentry_resets(c);
	test_partial_empty_and_unavailable_native_context(c);
}
