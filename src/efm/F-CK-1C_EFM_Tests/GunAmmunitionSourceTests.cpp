#include "TestHarness.h"
#include "TestFileUtils.h"
#include "Fck1cEfmTestFixture.h"
#include "DcsBridge/Internal/EventLog.h"
#include "DcsBridge/Internal/GunAmmunition/GunAmmunitionSource.h"

#include <stdexcept>

namespace
{
using namespace DcsBridge::Internal;

void test_sampling_and_recovery(Tests::Context& context)
{
	TestFiles::TemporaryDirectory root;
	EventLog log(root.path().string().c_str());
	unsigned calls = 0;
	NativeGunResult result{ NativeGunStatus::WaitingForPayload, 0, 0 };
	bool throw_read = false;
	GunAmmunitionSource source(log,
	                           [&]()
	                           {
		                           ++calls;
		                           if (throw_read)
			                           throw std::runtime_error("native reader failed");
		                           return result;
	                           });
	source.reset_flight();
	auto observation = source.observe(0);
	TEST_EXPECT(context, !observation.status.available && calls == 1);
	result = { NativeGunStatus::Available, 523, 1 };
	TEST_EXPECT(context, source.observe(0.05).status.revision == observation.status.revision);
	TEST_EXPECT(context, calls == 1);
	observation = source.observe(0.1);
	TEST_EXPECT(context, observation.status.available && observation.count == 523 && calls == 2);
	result.count = 0;
	observation = source.observe(0.21);
	TEST_EXPECT(context, observation.status.available && observation.count == 0 && calls == 3);
	result.count = 523;
	TEST_EXPECT(context, source.observe(0.32).count == 523 && calls == 4);
	result = { NativeGunStatus::InvalidCount, 10000, 1 };
	observation = source.observe(0.43);
	TEST_EXPECT(context, !observation.status.available);
	TEST_EXPECT(context, observation.status.invalid_reason == Core::ObservationInvalidReason::InvalidNumeric);
	result.status = NativeGunStatus::CallFailed;
	TEST_EXPECT(context, !source.observe(0.54).status.available && calls == 6);
	result = { NativeGunStatus::Available, 261, 1 };
	TEST_EXPECT(context, !source.observe(0.65).status.available && calls == 6);
	source.reset_flight();
	observation = source.observe(0);
	TEST_EXPECT(context, observation.status.available && observation.count == 261 && calls == 7);
	TEST_EXPECT(context, observation.status.revision == 1);
	throw_read = true;
	TEST_EXPECT(context, !source.observe(0.11).status.available && calls == 8);
	throw_read = false;
	TEST_EXPECT(context, !source.observe(0.22).status.available && calls == 8);
	source.reset_flight();
	TEST_EXPECT(context, source.observe(0).status.available && calls == 9);
	const auto text = TestFiles::read_text_while_open(root.path() / "log/fck1c_efm.log");
	TEST_EXPECT(context, text.find("status=waiting_for_payload") != std::string::npos);
	TEST_EXPECT(context, text.find("status=available") != std::string::npos);
	TEST_EXPECT(context, text.find("status=native_call_failed") != std::string::npos);
}

void test_stable_status_logging(Tests::Context& context)
{
	TestFiles::TemporaryDirectory root;
	EventLog log(root.path().string().c_str());
	GunAmmunitionSource source(log,
	                           []()
	                           {
		                           return NativeGunResult{ NativeGunStatus::Available, 523, 1 };
	                           });
	(void)source.observe(0);
	const auto first = TestFiles::read_text_while_open(root.path() / "log/fck1c_efm.log");
	(void)source.observe(0.11);
	(void)source.observe(0.22);
	TEST_EXPECT(context, !first.empty());
	TEST_EXPECT(context, TestFiles::read_text_while_open(root.path() / "log/fck1c_efm.log") == first);
}

void test_native_observation_reaches_mc(Tests::Context& context)
{
	TestFiles::TemporaryDirectory root;
	EventLog log(root.path().string().c_str());
	unsigned count = 523;
	GunAmmunitionSource source(log,
	                           [&]()
	                           {
		                           return NativeGunResult{ NativeGunStatus::Available, count, 1 };
	                           });
	Core::Fck1cEfm core(Tests::Fck1c::make_test_config(), Tests::disabled_debug_telemetry());
	(void)core.start(Core::StartMode::HotAir);
	auto input = Tests::Fck1c::make_frame_input();
	input.dt_s = 0.03125;
	double time_s = 0;
	Core::FrameOutput output;
	for (const unsigned rounds : { 523u, 464u, 407u, 340u, 269u, 212u, 0u, 523u })
	{
		count = rounds;
		for (int step = 0; step < 4; ++step)
		{
			input.cockpit.gun_ammunition = source.observe(time_s);
			output = core.step(input);
			time_s += input.dt_s;
		}
		const auto& gun = output.cockpit.combat_avionics.stores.gun_ammunition;
		TEST_EXPECT(context, gun.observation.available);
		TEST_EXPECT(context, gun.count == static_cast<int>(rounds));
		TEST_EXPECT(context, gun.source == Core::AmmunitionSource::Native);
	}
	core.release();
	source.reset_flight();
	(void)core.start(Core::StartMode::HotAir);
	count = 261;
	input.cockpit.gun_ammunition = source.observe(0);
	output = core.step(input);
	TEST_EXPECT(context, output.cockpit.combat_avionics.stores.gun_ammunition.count == 261);
}
}

void run_gun_ammunition_source_tests(Tests::Context& context)
{
	test_sampling_and_recovery(context);
	test_stable_status_logging(context);
	test_native_observation_reaches_mc(context);
}
