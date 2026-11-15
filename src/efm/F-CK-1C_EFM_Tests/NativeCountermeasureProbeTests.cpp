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
	unsigned reads = 0;
	NativeCmInventory inventory;
	std::vector<NativeCmCall> calls;
	std::vector<NativeCmProbeSnapshot> reports;
	std::function<void()> on_read, on_call;
	NativeCountermeasureProbe probe{ { [this]
		                               {
		                                   return mask;
		                               },
		                               [this](const NativeCmCall& call)
		                               {
		                                   calls.push_back(call);
		                                   NativeCmCallResult result;
		                                   result.before = inventory;
		                                   if ((call.kind == NativeCmKind::Flare ? inventory.flare : inventory.chaff) ==
		                                       0)
		                                   {
			                                   result.after = inventory;
			                                   result.after.status = NativeCmSourceStatus::NoAmmunition;
			                                   return result;
		                                   }
		                                   result.returned = succeeds;
		                                   result.exception = succeeds ? 0U : 0xc0000005U;
		                                   if (succeeds)
		                                   {
			                                   if (call.kind == NativeCmKind::Flare)
				                                   --inventory.flare;
			                                   else
				                                   --inventory.chaff;
		                                   }
		                                   result.after = inventory;
		                                   if (!succeeds)
		                                   {
			                                   result.after.available = false;
			                                   result.after.flare = result.after.chaff = -1;
			                                   result.after.status = NativeCmSourceStatus::NativeCallFailed;
			                                   result.after.exception = result.exception;
		                                   }
		                                   if (on_call)
			                                   on_call();
		                                   return result;
		                               },
		                               [this]
		                               {
		                                   return thread;
		                               },
		                               [this]
		                               {
		                                   ++reads;
		                                   if (on_read)
			                                   on_read();
		                                   return inventory;
		                               } },
		                             [this](const NativeCmProbeSnapshot& value, const char*)
		                             {
		                                 reports.push_back(value);
		                             } };
	Fixture()
	{
		inventory.available = inventory.payload_matches = true;
		inventory.status = NativeCmSourceStatus::Available;
		inventory.flare = inventory.chaff = 90;
	}
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
		f.press(NativeCmFlareDirect);
		f.probe.step(1, true);
		TEST_EXPECT(context, f.calls.empty() && f.reads == 0 && f.reports.empty());
		TEST_EXPECT(context, !f.probe.handle_command(-1, 1));
	}
	{
		Fixture f;
		f.probe.begin_flight(true);
		f.probe.step(1, false);
		TEST_EXPECT(context, f.reads == 1 && f.probe.snapshot().inventory.flare == 90);
		TEST_EXPECT(context, f.probe.snapshot().api_mask == 3 && f.probe.snapshot().samples == 1);
		f.probe.step(1.099, false);
		TEST_EXPECT(context, f.reads == 1);
		f.inventory.flare = f.inventory.chaff = 0;
		f.probe.step(1.101, false);
		TEST_EXPECT(context, f.reads == 2 && f.probe.snapshot().inventory.available);
		TEST_EXPECT(context, f.probe.snapshot().inventory.flare == 0 && f.calls.empty());
	}
	{
		Fixture f;
		f.probe.begin_flight(true);
		(void)f.probe.handle_command(NativeCmFlareDirect, 1);
		(void)f.probe.handle_command(NativeCmFlareDirect, 1);
		(void)f.probe.handle_command(NativeCmFlareDirect, std::numeric_limits<float>::quiet_NaN());
		TEST_EXPECT(context, f.probe.snapshot().requests == 1 && f.calls.empty());
		f.thread = 20;
		f.probe.step(1, true);
		TEST_EXPECT(context, f.calls.size() == 1 && f.calls[0].kind == NativeCmKind::Flare);
		TEST_EXPECT(context, f.probe.snapshot().request_thread == 10 && f.probe.snapshot().execute_thread == 20);
		TEST_EXPECT(context, f.probe.snapshot().before_flare == 90 && f.probe.snapshot().inventory.flare == 89);
		f.probe.step(2, true);
		TEST_EXPECT(context, f.calls.size() == 1);
		f.press(NativeCmChaffDirect);
		f.probe.step(3, true);
		TEST_EXPECT(context, f.calls.size() == 2 && f.calls[1].kind == NativeCmKind::Chaff);
		TEST_EXPECT(context, f.probe.snapshot().inventory.chaff == 89 && f.probe.snapshot().inventory.flare == 89);
	}
	{
		Fixture f;
		f.probe.begin_flight(true);
		f.press(NativeCmFlareDirect);
		f.press(NativeCmChaffDirect);
		f.probe.step(1, false);
		TEST_EXPECT(context, f.probe.snapshot().status == NativeCmStatus::NotArmed && f.calls.empty());
		f.probe.step(2, true);
		TEST_EXPECT(context, f.calls.empty());
		f.press(NativeCmChaffDirect);
		f.probe.step(3, true);
		TEST_EXPECT(context, f.calls.size() == 1 && f.calls[0].kind == NativeCmKind::Chaff);
	}
	{
		Fixture f;
		f.mask = 0;
		f.probe.begin_flight(true);
		f.press(NativeCmFlareDirect);
		f.probe.step(1, true);
		TEST_EXPECT(context, f.calls.empty() && f.reads == 0);
		TEST_EXPECT(context, f.probe.snapshot().status == NativeCmStatus::MissingExport);
	}
	{
		Fixture f;
		f.inventory.available = false;
		f.inventory.status = NativeCmSourceStatus::WaitingForHuman;
		f.probe.begin_flight(true);
		f.press(NativeCmFlareDirect);
		f.probe.step(1, true);
		TEST_EXPECT(context, f.calls.empty() && f.probe.snapshot().status == NativeCmStatus::WaitingForSource);
		f.inventory.available = true;
		f.inventory.status = NativeCmSourceStatus::Available;
		f.probe.step(2, true);
		TEST_EXPECT(context, f.calls.empty() && f.probe.snapshot().status == NativeCmStatus::Ready);
		f.press(NativeCmFlareDirect);
		f.probe.step(3, true);
		TEST_EXPECT(context, f.calls.size() == 1);
	}
	{
		Fixture f;
		f.probe.begin_flight(true);
		f.succeeds = false;
		f.press(NativeCmFlareDirect);
		f.probe.step(1, true);
		TEST_EXPECT(context, f.probe.snapshot().status == NativeCmStatus::CallFailed);
		TEST_EXPECT(context, f.probe.snapshot().exception == 0xc0000005U && f.probe.snapshot().calls == 0);
		TEST_EXPECT(context, f.probe.snapshot().before_flare == 90 && f.probe.snapshot().before_chaff == 90);
		TEST_EXPECT(context, !f.probe.snapshot().inventory.available && f.probe.snapshot().inventory.flare == -1);
		const auto reads = f.reads;
		f.press(NativeCmChaffDirect);
		f.probe.step(2, true);
		TEST_EXPECT(context, f.calls.size() == 1 && f.reads == reads);
		f.succeeds = true;
		f.probe.begin_flight(true);
		f.press(NativeCmChaffDirect);
		f.probe.step(3, true);
		TEST_EXPECT(context, f.calls.size() == 2);
	}
	{
		Fixture f;
		f.probe.begin_flight(true);
		f.press(NativeCmFlareDirect);
		f.probe.step(1, true);
		f.succeeds = false;
		f.press(NativeCmChaffDirect);
		f.probe.step(2, true);
		TEST_EXPECT(context, f.probe.snapshot().before_flare == 89 && f.probe.snapshot().before_chaff == 90);
		TEST_EXPECT(context, !f.probe.snapshot().inventory.available && f.probe.snapshot().inventory.chaff == -1);
	}
	{
		Fixture f;
		f.inventory.flare = 0;
		f.probe.begin_flight(true);
		f.press(NativeCmFlareDirect);
		f.probe.step(1, true);
		TEST_EXPECT(context,
		            f.probe.snapshot().status == NativeCmStatus::NoAmmunition && f.probe.snapshot().calls == 0);
		f.press(NativeCmChaffDirect);
		f.probe.step(2, true);
		TEST_EXPECT(context, f.probe.snapshot().calls == 1 && f.probe.snapshot().inventory.chaff == 89);
	}
	{
		Fixture f;
		f.inventory.available = false;
		f.inventory.exception = 0xc0000005U;
		f.probe.begin_flight(true);
		f.probe.step(1, true);
		f.probe.step(2, true);
		TEST_EXPECT(context, f.reads == 1 && f.probe.snapshot().status == NativeCmStatus::CallFailed);
	}
	{
		Fixture f;
		f.probe.begin_flight(true);
		f.press(NativeCmFlareDirect);
		f.on_call = [&f]
		{
			(void)f.probe.snapshot();
			f.press(NativeCmChaffDirect);
		};
		f.probe.step(1, true);
		TEST_EXPECT(context, f.probe.snapshot().requests == 2);
		f.on_call = {};
		f.probe.step(2, true);
		TEST_EXPECT(context, f.calls.size() == 2 && f.calls[1].kind == NativeCmKind::Chaff);
	}
	{
		Fixture f;
		f.probe.begin_flight(true);
		f.press(NativeCmFlareDirect);
		f.on_read = [&f]
		{
			if (f.reads == 1)
			{
				f.probe.end_flight();
				f.probe.begin_flight(true);
				f.probe.step(1.5, true);
			}
		};
		f.probe.step(1, true);
		TEST_EXPECT(context, f.calls.empty() && f.probe.snapshot().samples == 0);
		TEST_EXPECT(context, f.reads == 1);
		f.on_read = {};
		f.probe.step(2, true);
		TEST_EXPECT(context, f.calls.empty() && f.probe.snapshot().samples == 1);
	}
	{
		Fixture f;
		f.probe.begin_flight(true);
		f.press(NativeCmFlareDirect);
		f.on_call = [&f]
		{
			f.probe.end_flight();
			f.probe.begin_flight(true);
			f.probe.step(1.5, true);
		};
		f.probe.step(1, true);
		TEST_EXPECT(context, f.calls.size() == 1 && f.probe.snapshot().calls == 0);
		TEST_EXPECT(context, f.probe.snapshot().samples == 0);
		f.on_call = {};
		f.probe.step(2, true);
		TEST_EXPECT(context, f.calls.size() == 1);
	}
	{
		Fixture f;
		f.probe.begin_flight(true);
		for (int i = 0; i < 17; ++i)
			f.press(NativeCmFlareDirect);
		TEST_EXPECT(context,
		            f.probe.snapshot().status == NativeCmStatus::QueueFull && f.probe.snapshot().requests == 16);
		for (int i = 0; i < 20; ++i)
			f.probe.step(i, true);
		TEST_EXPECT(context, f.calls.size() == 16 && f.probe.snapshot().inventory.flare == 74);
	}
}
