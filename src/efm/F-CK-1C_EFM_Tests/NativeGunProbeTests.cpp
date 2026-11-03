#include "TestHarness.h"
#include "TestFileUtils.h"
#include "DcsBridge/Internal/EventLog.h"
#include "DcsBridge/Internal/NativeGunProbe.h"

#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

namespace
{
using namespace DcsBridge::Internal;

void test_disabled_probe(Tests::Context& context)
{
	TestFiles::TemporaryDirectory root;
	EventLog log(root.path().string().c_str());
	unsigned calls = 0;
	NativeGunProbe probe(root.path().string(),
	                     log,
	                     [&]()
	                     {
		                     ++calls;
		                     return NativeGunResult{ NativeGunStatus::Available, 523, 1 };
	                     });
	probe.reset_flight();
	TEST_EXPECT(context, !probe.enabled());
	TEST_EXPECT(context, !probe.observe(0).status.available);
	probe.record_mc(0, {});
	TEST_EXPECT(context, calls == 0);
	TEST_EXPECT(context, TestFiles::read_text_while_open(root.path() / "log/fck1c_efm.log").empty());
}

void test_read_pacing_and_flight_recovery(Tests::Context& context)
{
	TestFiles::TemporaryDirectory root;
	std::filesystem::create_directory(root.path() / "FM");
	TestFiles::write_text(root.path() / "FM/native-gun-probe.enabled", "");
	EventLog log(root.path().string().c_str());
	unsigned calls = 0;
	NativeGunResult result{ NativeGunStatus::WaitingForPayload, 0, 0 };
	bool throw_read = false;
	NativeGunProbe probe(root.path().string(),
	                     log,
	                     [&]()
	                     {
		                     ++calls;
		                     if (throw_read)
			                     throw std::runtime_error("diagnostic read failed");
		                     return result;
	                     });
	probe.reset_flight();
	TEST_EXPECT(context, probe.enabled());
	auto observation = probe.observe(0);
	TEST_EXPECT(context, !observation.status.available && calls == 1);
	result = { NativeGunStatus::Available, 523, 1 };
	TEST_EXPECT(context, probe.observe(0.05).status.revision == observation.status.revision);
	TEST_EXPECT(context, calls == 1);
	observation = probe.observe(0.1);
	TEST_EXPECT(context, observation.status.available && observation.count == 523 && calls == 2);
	Core::CombatAvionicsSnapshot snapshot;
	snapshot.stores.gun_ammunition.observation = observation.status;
	snapshot.stores.gun_ammunition.count = observation.count;
	probe.record_mc(0.101, snapshot);
	probe.record_mc(0.102, snapshot);
	const auto text = TestFiles::read_text_while_open(root.path() / "log/fck1c_efm.log");
	std::istringstream lines(text);
	std::string line;
	unsigned samples = 0;
	while (std::getline(lines, line))
	{
		const auto offset = line.find("native_gun_probe ");
		if (offset == std::string::npos)
			continue;
		const auto record = nlohmann::json::parse(line.substr(offset + 17));
		if (record["event"] != "sample")
			continue;
		++samples;
		TEST_EXPECT(context, record["raw_count"] == 523 && record["mc_count"] == 523);
		TEST_EXPECT(context, record["raw_available"] == true && record["mc_available"] == true);
		TEST_EXPECT(context, record["flight"] == 1);
	}
	TEST_EXPECT(context, samples == 1);
	result.count = 0;
	observation = probe.observe(0.21);
	TEST_EXPECT(context, observation.status.available && observation.count == 0 && calls == 3);
	result.status = NativeGunStatus::CallFailed;
	TEST_EXPECT(context, !probe.observe(0.32).status.available && calls == 4);
	result = { NativeGunStatus::Available, 261, 1 };
	TEST_EXPECT(context, !probe.observe(0.43).status.available && calls == 4);
	probe.reset_flight();
	observation = probe.observe(0);
	TEST_EXPECT(context, observation.status.available && observation.count == 261 && calls == 5);
	TEST_EXPECT(context, observation.status.revision == 2);
	throw_read = true;
	TEST_EXPECT(context, !probe.observe(0.11).status.available && calls == 6);
	throw_read = false;
	TEST_EXPECT(context, !probe.observe(0.22).status.available && calls == 6);
	probe.reset_flight();
	TEST_EXPECT(context, probe.observe(0).status.available && calls == 7);
}
}

void run_native_gun_probe_tests(Tests::Context& context)
{
	test_disabled_probe(context);
	test_read_pacing_and_flight_recovery(context);
}
