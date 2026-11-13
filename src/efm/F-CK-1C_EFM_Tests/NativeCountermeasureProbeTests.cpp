#include "TestHarness.h"
#include "DcsBridge/Internal/NativeCountermeasures/NativeCountermeasureProbe.h"
#include "DcsIds/CustomCommands.g.h"

#include <limits>
#include <vector>

namespace
{
using namespace DcsBridge::Internal;
struct Fixture
{
	unsigned mask = 3;
	bool succeeds = true;
	std::uint32_t thread = 10;
	std::vector<NativeCmCall> calls;
	std::vector<NativeCmProbeSnapshot> reports;
	std::function<void()> callback;
	NativeCountermeasureProbe probe{ { [this]
		                               {
		                                   return mask;
		                               },
		                               [this](const NativeCmCall& call)
		                               {
		                                   calls.push_back(call);
		                                   if (callback)
			                                   callback();
		                                   return NativeCmCallResult{ succeeds, succeeds ? 0U : 0xc0000005U };
		                               },
		                               [this]
		                               {
		                                   return thread;
		                               } },
		                             [this](const NativeCmProbeSnapshot& snapshot, const char*)
		                             {
		                                 reports.push_back(snapshot);
		                             } };
	void press(int id)
	{
		(void)probe.handle_command(id, 1);
		(void)probe.handle_command(id, 0);
	}
};
}
void run_native_countermeasure_probe_tests(Tests::Context& context)
{
	using namespace DcsIds::Commands;
	using namespace DcsBridge::Internal;
	{
		Fixture f;
		f.probe.begin_flight(false);
		f.press(NativeCmFlareDigital);
		f.probe.step(1, true);
		TEST_EXPECT(context, f.calls.empty());
		TEST_EXPECT(context, f.reports.empty());
		TEST_EXPECT(context, !f.probe.handle_command(-1, 1));
	}
	{
		Fixture f;
		f.probe.begin_flight(true);
		TEST_EXPECT(context, f.probe.snapshot().api_mask == 3);
		(void)f.probe.handle_command(NativeCmFlareDigital, 1);
		(void)f.probe.handle_command(NativeCmFlareDigital, 1);
		(void)f.probe.handle_command(NativeCmFlareDigital, std::numeric_limits<float>::quiet_NaN());
		TEST_EXPECT(context, f.probe.snapshot().requests == 1);
		TEST_EXPECT(context, f.calls.empty());
		f.thread = 20;
		f.probe.step(1, true);
		TEST_EXPECT(context, f.calls.size() == 1);
		TEST_EXPECT(context, f.calls[0].command == DcsIds::DcsCommands::iCommandPlaneDropFlareOnce);
		TEST_EXPECT(context, f.calls[0].method == NativeCmMethod::Digital);
		TEST_EXPECT(context, f.probe.snapshot().request_thread == 10);
		TEST_EXPECT(context, f.probe.snapshot().execute_thread == 20);
		f.probe.step(2, true);
		TEST_EXPECT(context, f.calls.size() == 1);
		f.press(NativeCmChaffDigital);
		f.probe.step(3, true);
		TEST_EXPECT(context, f.calls.size() == 2);
		TEST_EXPECT(context, f.calls[1].command == DcsIds::DcsCommands::iCommandPlaneDropChaffOnce);
	}
	{
		Fixture f;
		f.probe.begin_flight(true);
		f.press(NativeCmFlareAnalog);
		f.press(NativeCmChaffAnalog);
		f.probe.step(1, true);
		f.probe.step(1.039, true);
		TEST_EXPECT(context, f.calls.size() == 1);
		f.probe.step(1.041, true);
		TEST_EXPECT(context, f.calls.size() == 2);
		TEST_EXPECT(context, f.calls[1].value == 0);
		TEST_EXPECT(context, f.calls[1].request == f.calls[0].request);
		TEST_EXPECT(context, f.probe.snapshot().status == NativeCmStatus::Released);
		f.probe.step(1.05, true);
		TEST_EXPECT(context, f.calls.size() == 3);
		TEST_EXPECT(context, f.calls[2].kind == NativeCmKind::Chaff);
		f.probe.step(1.1, true);
		f.probe.step(2, true);
		TEST_EXPECT(context, f.calls.size() == 4);
	}
	{
		Fixture f;
		f.mask = 1;
		f.probe.begin_flight(true);
		f.press(NativeCmFlareAnalog);
		f.probe.step(1, true);
		TEST_EXPECT(context, f.calls.empty());
		TEST_EXPECT(context, f.probe.snapshot().status == NativeCmStatus::MissingExport);
		f.press(NativeCmChaffDigital);
		f.probe.step(2, true);
		TEST_EXPECT(context, f.calls.size() == 1);
	}
	{
		Fixture f;
		f.probe.begin_flight(true);
		f.press(NativeCmFlareDigital);
		f.press(NativeCmChaffDigital);
		f.probe.step(1, false);
		TEST_EXPECT(context, f.probe.snapshot().status == NativeCmStatus::NotArmed);
		f.probe.step(2, true);
		TEST_EXPECT(context, f.calls.empty());
		f.press(NativeCmFlareAnalog);
		f.probe.step(3, true);
		f.press(NativeCmChaffDigital);
		f.probe.step(3.001, false);
		TEST_EXPECT(context, f.calls.size() == 2);
		TEST_EXPECT(context, f.calls.back().value == 0);
		f.probe.step(4, true);
		TEST_EXPECT(context, f.calls.size() == 2);
	}
	{
		Fixture f;
		f.probe.begin_flight(true);
		f.succeeds = false;
		f.press(NativeCmFlareDigital);
		f.probe.step(1, true);
		TEST_EXPECT(context, f.probe.snapshot().status == NativeCmStatus::CallFailed);
		TEST_EXPECT(context, f.probe.snapshot().exception == 0xc0000005U);
		TEST_EXPECT(context, f.probe.snapshot().calls == 0);
		f.press(NativeCmChaffDigital);
		f.probe.step(2, true);
		TEST_EXPECT(context, f.calls.size() == 1);
		f.succeeds = true;
		f.probe.begin_flight(true);
		f.press(NativeCmChaffDigital);
		f.probe.step(3, true);
		TEST_EXPECT(context, f.calls.size() == 2);
	}
	{
		Fixture f;
		f.probe.begin_flight(true);
		f.press(NativeCmFlareDigital);
		f.callback = [&f]
		{
			f.press(DcsIds::Commands::NativeCmChaffDigital);
		};
		f.probe.step(1, true);
		TEST_EXPECT(context, f.probe.snapshot().requests == 2);
		f.callback = {};
		f.probe.step(2, true);
		TEST_EXPECT(context, f.calls.size() == 2);
		TEST_EXPECT(context, f.calls[1].kind == NativeCmKind::Chaff);
	}
	{
		Fixture f;
		f.probe.begin_flight(true);
		f.press(NativeCmFlareAnalog);
		f.callback = [&f]
		{
			f.probe.end_flight();
			f.probe.begin_flight(true);
		};
		f.probe.step(1, true);
		TEST_EXPECT(context, f.probe.snapshot().calls == 0);
		f.callback = {};
		f.probe.step(2, true);
		TEST_EXPECT(context, f.calls.size() == 1);
		f.press(NativeCmChaffDigital);
		f.probe.end_flight();
		f.probe.begin_flight(true);
		f.probe.step(3, true);
		TEST_EXPECT(context, f.calls.size() == 1);
		TEST_EXPECT(context, f.probe.snapshot().requests == 0);
	}
	{
		Fixture f;
		f.probe.begin_flight(true);
		for (int i = 0; i < 17; ++i)
			f.press(NativeCmFlareDigital);
		TEST_EXPECT(context, f.probe.snapshot().status == NativeCmStatus::QueueFull);
		TEST_EXPECT(context, f.probe.snapshot().requests == 16);
		for (int i = 0; i < 20; ++i)
			f.probe.step(i, true);
		TEST_EXPECT(context, f.calls.size() == 16);
	}
}
