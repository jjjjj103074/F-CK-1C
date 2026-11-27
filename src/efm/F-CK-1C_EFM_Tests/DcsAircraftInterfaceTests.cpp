#include "TestHarness.h"
#include "DcsAircraftTestFixture.h"
#include "DcsBridge/Internal/Aircraft/DcsAircraftInterface.h"
#include "DcsBridge/Internal/Aircraft/DcsAircraftBackend.h"

namespace
{
using namespace DcsBridge::Internal;
using Fixture = Tests::Aircraft::Fixture;

void test_shared_access(Tests::Context& t)
{
	Fixture f;
	DcsAircraftInterface aircraft(f.backend());
	auto gun = aircraft.read_gun_ammunition();
	TEST_EXPECT(t, gun.ammunition.status == NativeGunStatus::Available && gun.ammunition.count == 523);
	TEST_EXPECT(t, gun.build.gun_profile_matched && gun.build.payload_export && gun.exception == 0);
	auto stock = aircraft.read_countermeasures();
	TEST_EXPECT(t, stock.available() && stock.flare == 90 && stock.chaff == 90);
	TEST_EXPECT(t, f.fingerprints == 6);
	auto shot = aircraft.dispense_countermeasure(AircraftCountermeasure::Flare);
	TEST_EXPECT(t, shot.returned && shot.before.flare == 90 && shot.after.flare == 89 && shot.after.chaff == 90);
	shot = aircraft.dispense_countermeasure(AircraftCountermeasure::Chaff);
	TEST_EXPECT(t, shot.returned && shot.before.chaff == 90 && shot.after.chaff == 89 && shot.after.flare == 89);
	f.gun = 0;
	TEST_EXPECT(t, aircraft.read_gun_ammunition().ammunition.count == 0);
	f.flare = 0;
	shot = aircraft.dispense_countermeasure(AircraftCountermeasure::Flare);
	TEST_EXPECT(t, !shot.returned && shot.after.status == AircraftAccessStatus::NoAmmunition && f.drops == 2);
	TEST_EXPECT(t, f.fingerprints == 6);
	aircraft.reset_flight();
	stock = aircraft.read_countermeasures();
	TEST_EXPECT(t, stock.available() && stock.flare == 0 && stock.chaff == 89);
	TEST_EXPECT(t, f.fingerprints == 11);
}
void test_current_build_access(Tests::Context& t)
{
	Fixture f;
	f.use_current_build();
	DcsAircraftInterface aircraft(f.backend());
	const auto gun = aircraft.read_gun_ammunition();
	TEST_EXPECT(t, gun.ammunition.status == NativeGunStatus::Available && gun.ammunition.count == 523);
	TEST_EXPECT(t, gun.build.gun_profile_matched);
	const auto stock = aircraft.read_countermeasures();
	TEST_EXPECT(t, stock.available() && stock.flare == 90 && stock.chaff == 90);
	const auto flare = aircraft.dispense_countermeasure(AircraftCountermeasure::Flare);
	TEST_EXPECT(t, flare.returned && flare.after.flare == 89 && flare.after.chaff == 90);
	const auto chaff = aircraft.dispense_countermeasure(AircraftCountermeasure::Chaff);
	TEST_EXPECT(t, chaff.returned && chaff.after.flare == 89 && chaff.after.chaff == 89);

	Fixture wrong;
	wrong.use_current_build();
	wrong.hashes[0x5200] ^= 1;
	DcsAircraftInterface invalid(wrong.backend());
	TEST_EXPECT(t, invalid.read_countermeasures().status == AircraftAccessStatus::UnsupportedBuild);
	TEST_EXPECT(t, wrong.calls == 0);
	TEST_EXPECT(t, invalid.read_gun_ammunition().ammunition.status == NativeGunStatus::Available);

	Fixture changed;
	DcsAircraftInterface reloaded(changed.backend());
	TEST_EXPECT(t, reloaded.read_countermeasures().available());
	changed.use_current_build();
	TEST_EXPECT(t, reloaded.read_countermeasures().available());
	TEST_EXPECT(t, reloaded.read_gun_ammunition().build.gun_profile_matched);
}
void test_capability_checks(Tests::Context& t)
{
	Fixture f;
	f.hashes[0x5200] ^= 1;
	DcsAircraftInterface aircraft(f.backend());
	TEST_EXPECT(t, aircraft.read_countermeasures().status == AircraftAccessStatus::UnsupportedBuild);
	TEST_EXPECT(t, f.calls == 0);
	TEST_EXPECT(t, aircraft.read_gun_ammunition().ammunition.count == 523);
	f.hashes[0x5200] ^= 1;
	aircraft.reset_flight();
	TEST_EXPECT(t, aircraft.read_countermeasures().available());
	f.module.timestamp ^= 1;
	const auto calls = f.calls;
	TEST_EXPECT(t, aircraft.read_countermeasures().status == AircraftAccessStatus::UnsupportedBuild);
	TEST_EXPECT(t, aircraft.read_gun_ammunition().ammunition.status == NativeGunStatus::UnsupportedBuild);
	TEST_EXPECT(t, calls == f.calls);
	f.module.base = 0;
	TEST_EXPECT(t, aircraft.read_countermeasures().status == AircraftAccessStatus::WaitingForModule);
	TEST_EXPECT(t, aircraft.read_gun_ammunition().ammunition.status == NativeGunStatus::WaitingForModule);
	Fixture invalid;
	invalid.module.valid = false;
	DcsAircraftInterface invalid_header(invalid.backend());
	const auto header = invalid_header.read_gun_ammunition();
	TEST_EXPECT(t,
	            header.build.present && !header.build.header_valid &&
	                header.ammunition.status == NativeGunStatus::UnsupportedBuild && invalid.calls == 0);
	Fixture missing;
	missing.symbols.erase("?contexts_ptr@ccCockpitContext@cockpit@@0PAPEAV12@A");
	DcsAircraftInterface no_context(missing.backend());
	TEST_EXPECT(t,
	            no_context.read_countermeasures().status == AircraftAccessStatus::MissingInterface &&
	                missing.calls == 0);
	Fixture nx;
	nx.non_executable = true;
	DcsAircraftInterface not_executable(nx.backend());
	TEST_EXPECT(t,
	            not_executable.read_countermeasures().status == AircraftAccessStatus::UnsupportedBuild &&
	                nx.calls == 0);
}
void test_aircraft_lifecycle(Tests::Context& t)
{
	Fixture f;
	f.memory[0x6000] = 0;
	DcsAircraftInterface aircraft(f.backend());
	TEST_EXPECT(t, aircraft.read_countermeasures().status == AircraftAccessStatus::WaitingForContext);
	f.memory[0x6000] = 0x1000;
	f.memory[0x1010] = 0;
	TEST_EXPECT(t, aircraft.read_countermeasures().status == AircraftAccessStatus::WaitingForHuman);
	f.memory[0x1010] = 0x2000;
	TEST_EXPECT(t, aircraft.read_countermeasures().available());
	f.mismatch = true;
	TEST_EXPECT(t, aircraft.read_countermeasures().status == AircraftAccessStatus::PayloadMismatch);
	TEST_EXPECT(t, !aircraft.dispense_countermeasure(AircraftCountermeasure::Flare).returned && f.drops == 0);
	Fixture after_change;
	auto changed_backend = after_change.backend();
	const auto drop = changed_backend.dispense;
	changed_backend.dispense = [&](std::uintptr_t f, std::uintptr_t h, std::uint32_t& e)
	{
		const bool returned = drop(f, h, e);
		after_change.memory[0x1010] = 0x2010;
		return returned;
	};
	DcsAircraftInterface changed_after(changed_backend);
	const auto changed_result = changed_after.dispense_countermeasure(AircraftCountermeasure::Flare);
	TEST_EXPECT(t,
	            changed_result.returned && changed_result.after.status == AircraftAccessStatus::ContextChanged &&
	                changed_result.after.flare == -1);
	Fixture change;
	change.change_human = true;
	DcsAircraftInterface changing(change.backend());
	const auto shot = changing.dispense_countermeasure(AircraftCountermeasure::Flare);
	TEST_EXPECT(t, !shot.returned && shot.after.status == AircraftAccessStatus::ContextChanged && change.drops == 0);
}
void test_call_results(Tests::Context& t)
{
	Fixture f;
	f.fail_count = true;
	DcsAircraftInterface aircraft(f.backend());
	auto stock = aircraft.read_countermeasures();
	TEST_EXPECT(t,
	            stock.status == AircraftAccessStatus::CallFailed && stock.exception == 0xc0000005 &&
	                stock.flare == -1 && stock.chaff == -1);
	f.fail_count = false;
	f.flare = 10000;
	TEST_EXPECT(t, aircraft.read_countermeasures().status == AircraftAccessStatus::InvalidCount);
	f.flare = 90;
	f.fail_drop = true;
	const auto shot = aircraft.dispense_countermeasure(AircraftCountermeasure::Flare);
	TEST_EXPECT(t,
	            !shot.returned && shot.before.flare == 90 && shot.after.status == AircraftAccessStatus::CallFailed &&
	                shot.after.exception == 0xc0000005 && shot.after.flare == -1);
	TEST_EXPECT(t,
	            aircraft.dispense_countermeasure(static_cast<AircraftCountermeasure>(42)).after.status ==
	                AircraftAccessStatus::InvalidRequest);
	DcsAircraftInterface absent(DcsAircraftBackend{});
	TEST_EXPECT(t, absent.read_countermeasures().status == AircraftAccessStatus::WaitingForModule);
	TEST_EXPECT(t, absent.read_gun_ammunition().ammunition.status == NativeGunStatus::WaitingForModule);
	const unsigned char hello[] = { 'h', 'e', 'l', 'l', 'o' };
	TEST_EXPECT(t, aircraft_code_fingerprint(hello, sizeof(hello)) == 0xa430d84680aabd0bULL);
	TEST_EXPECT(t, aircraft_code_fingerprint(nullptr, 1) == 0);
}
}
void run_dcs_aircraft_interface_tests(Tests::Context& context)
{
	test_shared_access(context);
	test_current_build_access(context);
	test_capability_checks(context);
	test_aircraft_lifecycle(context);
	test_call_results(context);
}
