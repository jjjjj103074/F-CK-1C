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
	mc.infrared_missile.requested_tone = Core::InfraredSeekerTone::Lock;
	mc.infrared_missile.locked = true;
	mc.infrared_missile.contact = true;
	mc.infrared_missile.active = true;
	mc.infrared_missile.seeker_state = Core::InfraredSeekerState::Track;
	mc.infrared_missile.seeker_azimuth_rad = 0.1;
	mc.infrared_missile.seeker_elevation_rad = -0.2;
	mc.infrared_missile.lock_range_m = 1200.0;
	mc.stores.air_to_air_missile_count = 2;
	mc.stores.gun_count = 240.0;
	mc.control_inputs.trigger_second_stage_held = true;
	mc.control_inputs.master_arm_press_sequence = 3;
	mc.control_inputs.navigation_press_sequence = 5;
	mc.control_inputs.dogfight_press_sequence = 6;
	mc.control_inputs.missile_override_press_sequence = 8;
	mc.control_inputs.missile_uncage_press_sequence = 9;
	mc.control_inputs.weapon_release_press_sequence = 12;
	cockpit.set(DcsIds::CockpitParams::HmcsWeaponClass, 5.0);
	cockpit.set(DcsIds::CockpitParams::HmcsWeaponQuantity, 99.0);
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
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::HmcsWeaponClass), 5.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::HmcsWeaponQuantity), 99.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::WeaponSelectionRequestRevision), 4.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::Aim9MissileCount), 2.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McStoresGunCount), 240.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McTriggerSecondStageHeld), 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McMasterArmPressSequence), 3.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McNavigationPressSequence), 5.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McDogfightPressSequence), 6.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McMissileOverridePressSequence), 8.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McMissileUncagePressSequence), 9.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McWeaponReleasePressSequence), 12.0, kTolerance);
	TEST_EXPECT_NEAR(context,
	                 cockpit.value(DcsIds::CockpitParams::McAction01),
	                 static_cast<double>(Core::MissionActionKind::GunOn),
	                 kTolerance);
	TEST_EXPECT_NEAR(context,
	                 cockpit.value(DcsIds::CockpitParams::McAction02),
	                 static_cast<double>(Core::MissionActionKind::GunOff),
	                 kTolerance);
}

void test_missing_control_fact_blocks_mc_commit(Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	cockpit.set_available(DcsIds::CockpitParams::McMissileUncagePressSequence, false);
	DcsBridge::Internal::CockpitSnapshotExporter exporter(cockpit.api());
	Core::CockpitSnapshot snapshot;
	snapshot.combat_avionics.status = { true, 1 };
	TEST_EXPECT(context, exporter.export_snapshot(snapshot).count == 1);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McGeneration), 1.0, kTolerance);
	cockpit.set_available(DcsIds::CockpitParams::McMissileUncagePressSequence, true);
	TEST_EXPECT(context, exporter.export_snapshot(snapshot).count == 1);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McGeneration), 4.0, kTolerance);
}

void test_exporter_reports_all_missing_fields_and_recoveries(Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	for (const auto& entry : DcsIds::CockpitParams::Catalog)
		cockpit.set_available(entry.name, false);
	// A writable generation lets the exporter attempt every MC field.
	cockpit.set_available(DcsIds::CockpitParams::McGeneration, true);
	DcsBridge::Internal::CockpitSnapshotExporter exporter(cockpit.api());
	Core::CockpitSnapshot snapshot;
	snapshot.combat_avionics.status = { true, 1 };
	auto events = exporter.export_snapshot(snapshot);
	TEST_EXPECT(context, events.count == 67);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McGeneration), 1.0, kTolerance);
	TEST_EXPECT(context, exporter.export_snapshot(snapshot).count == 0); // Repeated failures remain suppressed.
	for (const auto& entry : DcsIds::CockpitParams::Catalog)
		cockpit.set_available(entry.name, true);
	events = exporter.export_snapshot(snapshot);
	TEST_EXPECT(context, events.count == 67);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McGeneration), 6.0, kTolerance);
	for (std::size_t i = 0; i < events.count; ++i)
		TEST_EXPECT(context, events.items[i].type == DcsBridge::Internal::CockpitParameterEventType::Recovery);
}

}

void run_cockpit_snapshot_exporter_tests(Tests::Context& context)
{
	test_snapshot_envelope_is_exported(context);
	test_missing_parameter_reports_once_then_recovers(context);
	test_reset_restarts_error_reporting(context);
	test_mc_window_commits_only_when_all_parameters_are_written(context);
	test_missing_control_fact_blocks_mc_commit(context);
	test_exporter_reports_all_missing_fields_and_recoveries(context);
}
