#include "FakeCockpitParameters.h"
#include "TestHarness.h"

#include "DcsBridge/Internal/CockpitSnapshotExporter.h"
#include "DcsIds/CockpitParams.g.h"

#include <cstring>

namespace
{
constexpr double kTolerance = 1e-9;

void test_snapshot_envelope_is_exported(Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	DcsBridge::Internal::CockpitSnapshotExporter exporter(cockpit.api());
	Core::CockpitSnapshot snapshot;
	snapshot.status = { true, 42 };
	snapshot.simulation_time_s = 12.5;
	snapshot.propulsion_test_thrust_cut_requested = true;
	const DcsBridge::Internal::CockpitParameterEvents events = exporter.export_snapshot(snapshot);
	TEST_EXPECT(context, events.count == 0);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::CockpitSnapshotAvailable), 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::CockpitSnapshotRevision), 42.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::CockpitSnapshotTimeS), 12.5, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::MaxPowerSwitch), 0.0, kTolerance);
}

void test_missing_parameter_reports_once_then_recovers(Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	cockpit.set_available(DcsIds::CockpitParams::CockpitSnapshotRevision, false);
	DcsBridge::Internal::CockpitSnapshotExporter exporter(cockpit.api());
	Core::CockpitSnapshot snapshot;
	snapshot.status = { true, 3 };
	DcsBridge::Internal::CockpitParameterEvents events = exporter.export_snapshot(snapshot);
	TEST_EXPECT(context, events.count == 1);
	if (events.count != 1)
	{
		return;
	}
	TEST_EXPECT(context,
	            events.items[0].parameter_name != nullptr &&
	                std::strcmp(events.items[0].parameter_name, DcsIds::CockpitParams::CockpitSnapshotRevision) == 0);
	TEST_EXPECT(context, events.items[0].type == DcsBridge::Internal::CockpitParameterEventType::Error);
	TEST_EXPECT(context, exporter.export_snapshot(snapshot).count == 0);
	cockpit.set_available(DcsIds::CockpitParams::CockpitSnapshotRevision, true);
	events = exporter.export_snapshot(snapshot);
	TEST_EXPECT(context, events.count == 1);
	if (events.count != 1)
	{
		return;
	}
	TEST_EXPECT(context, events.items[0].type == DcsBridge::Internal::CockpitParameterEventType::Recovery);
}

void test_reset_restarts_error_reporting(Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	cockpit.set_available(DcsIds::CockpitParams::CockpitSnapshotAvailable, false);
	DcsBridge::Internal::CockpitSnapshotExporter exporter(cockpit.api());
	TEST_EXPECT(context, exporter.export_snapshot({}).count == 1);
	TEST_EXPECT(context, exporter.export_snapshot({}).count == 0);
	exporter.reset();
	TEST_EXPECT(context, exporter.export_snapshot({}).count == 1);
}

void test_mc_window_commits_only_when_all_parameters_are_written(Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	cockpit.set_available(DcsIds::CockpitParams::McAction02, false);
	DcsBridge::Internal::CockpitSnapshotExporter exporter(cockpit.api());
	Core::CockpitSnapshot snapshot;
	auto& mc = snapshot.combat_avionics;
	mc.status = { true, 1 };
	mc.fire_control.master_arm_mode = Core::MasterArmMode::On;
	mc.fire_control.fire_control_mode = Core::FireControlMode::Dogfight;
	mc.aim9.requested_tone = Core::Aim9Tone::Lock;
	mc.aim9.locked = true;
	mc.aim9.contact = true;
	mc.aim9.active = true;
	mc.aim9.seeker_state = Core::Aim9SeekerState::Track;
	mc.aim9.seeker_azimuth_rad = 0.1;
	mc.aim9.seeker_elevation_rad = -0.2;
	mc.aim9.lock_range_m = 1200.0;
	mc.weapon_display.weapon_class = Core::WeaponClass::AirToAirMissile;
	mc.weapon_display.quantity = 2.0;
	mc.stores.requested_station = 3;
	mc.stores.selection_request_revision = 4;
	mc.external_actions.batch.first_sequence = 7;
	mc.external_actions.batch.count = 2;
	mc.external_actions.batch.actions[0] = Core::MissionActionKind::GunOn;
	mc.external_actions.batch.actions[1] = Core::MissionActionKind::GunOff;
	TEST_EXPECT(context, exporter.export_snapshot(snapshot).count == 1);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McGeneration), 1.0, kTolerance);
	cockpit.set_available(DcsIds::CockpitParams::McAction02, true);
	TEST_EXPECT(context, exporter.export_snapshot(snapshot).count == 1);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McGeneration), 4.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McActionFirstSequence), 7.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McActionCount), 2.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::WeaponRequestedStation), 3.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::Aim9ToneState), 3.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::Aim9SeekerState), 3.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::Aim9SeekerContact), 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::Aim9SeekerLock), 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::Aim9WeaponActive), 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::Aim9SeekerAzimuth), 0.1, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::Aim9SeekerElevation), -0.2, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::Aim9LockRange), 1200.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::HmcsWeaponClass), 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::HmcsWeaponQuantity), 2.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::WeaponSelectionRequestRevision), 4.0, kTolerance);
	TEST_EXPECT_NEAR(context,
	                 cockpit.value(DcsIds::CockpitParams::McAction01),
	                 static_cast<double>(Core::MissionActionKind::GunOn),
	                 kTolerance);
	TEST_EXPECT_NEAR(context,
	                 cockpit.value(DcsIds::CockpitParams::McAction02),
	                 static_cast<double>(Core::MissionActionKind::GunOff),
	                 kTolerance);
}
}

void run_cockpit_snapshot_exporter_tests(Tests::Context& context)
{
	test_snapshot_envelope_is_exported(context);
	test_missing_parameter_reports_once_then_recovers(context);
	test_reset_restarts_error_reporting(context);
	test_mc_window_commits_only_when_all_parameters_are_written(context);
}
