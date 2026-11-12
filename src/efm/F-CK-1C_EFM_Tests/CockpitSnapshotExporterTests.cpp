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
	Core::MissionActionBatch actions;
	snapshot.status = { true, 42 };
	snapshot.simulation_time_s = 12.5;
	snapshot.propulsion_test_thrust_cut_requested = true;
	const DcsBridge::Internal::CockpitParameterEvents events = exporter.export_snapshot(snapshot, actions);
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
	Core::MissionActionBatch actions;
	snapshot.status = { true, 3 };
	DcsBridge::Internal::CockpitParameterEvents events = exporter.export_snapshot(snapshot, actions);
	TEST_EXPECT(context, events.count == 1);
	if (events.count != 1)
	{
		return;
	}
	TEST_EXPECT(context,
	            events.items[0].parameter_name != nullptr &&
	                std::strcmp(events.items[0].parameter_name, DcsIds::CockpitParams::CockpitSnapshotRevision) == 0);
	TEST_EXPECT(context, events.items[0].type == DcsBridge::Internal::CockpitParameterEventType::Error);
	TEST_EXPECT(context, exporter.export_snapshot(snapshot, actions).count == 0);
	cockpit.set_available(DcsIds::CockpitParams::CockpitSnapshotRevision, true);
	events = exporter.export_snapshot(snapshot, actions);
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
	Core::MissionActionBatch actions;
	auto& mc = snapshot.combat_avionics;
	mc.status = { true, 1 };
	mc.mode.master_arm_mode = Core::MasterArmMode::On;
	mc.mode.operating_mode = Core::OperatingMode::Dogfight;
	mc.mode.dogfight_mode = true;
	mc.fire_control.missile_uncage_held = true;
	mc.stores.ir_seeker.status = { true, 12, Core::ObservationInvalidReason::None };
	mc.stores.ir_seeker.locked = true;
	mc.stores.ir_seeker.desired_azimuth_rad = 0.1;
	mc.stores.ir_seeker.desired_elevation_rad = -0.2;
	mc.stores.ir_seeker.target_azimuth_rad = 0.3;
	mc.stores.ir_seeker.target_elevation_rad = -0.4;
	mc.stores.air_to_air_missile_count = 2;
	mc.stores.air_to_air_missile_count_complete = true;
	mc.stores.gun_count = 240.0;
	mc.stores.gun_ammunition.observation.available = true;
	mc.control_inputs.trigger_second_stage_held = true;
	mc.control_inputs.master_arm_press_sequence = 3;
	mc.control_inputs.navigation_press_sequence = 5;
	mc.control_inputs.dogfight_press_sequence = 6;
	mc.control_inputs.missile_override_press_sequence = 8;
	mc.control_inputs.missile_uncage_press_sequence = 9;
	mc.control_inputs.weapon_release_press_sequence = 12;
	cockpit.set(DcsIds::CockpitParams::HmcsWeaponClass, 5.0);
	cockpit.set(DcsIds::CockpitParams::HmcsWeaponQuantity, 99.0);
	mc.stores.selection.requested_station = 3;
	mc.stores.selection.request_revision = 4;
	actions.first_sequence = 7;
	actions.count = 2;
	actions.actions[0].kind = Core::MissionActionKind::GunOn;
	actions.actions[1].kind = Core::MissionActionKind::GunOff;
	TEST_EXPECT(context, exporter.export_snapshot(snapshot, actions).count == 1);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McGeneration), 1.0, kTolerance);
	cockpit.set_available(DcsIds::CockpitParams::McAction02, true);
	TEST_EXPECT(context, exporter.export_snapshot(snapshot, actions).count == 1);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McGeneration), 4.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McActionFirstSequence), 7.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::HmcsMasterMode), 2.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::HmcsDogfightMode), 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::HmcsFireControlMode), 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McActionCount), 2.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::WeaponRequestedStation), -1.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McMissileUncageHeld), 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McStoresIrSeekerAvailable), 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McStoresIrSeekerRevision), 12.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McStoresIrSeekerInvalidReason), 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McStoresIrSeekerLock), 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McStoresIrDesiredAzimuth), 0.1, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McStoresIrDesiredElevation), -0.2, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McStoresIrTargetAzimuth), 0.3, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McStoresIrTargetElevation), -0.4, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::HmcsWeaponClass), 5.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::HmcsWeaponQuantity), 99.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::WeaponSelectionRequestRevision), 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McStoresAirToAirMissileCount), 2.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McStoresMissileCountAvailable), 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McStoresGunCount), 240.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McStoresGunCountAvailable), 1.0, kTolerance);
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
	Core::MissionActionBatch actions;
	snapshot.combat_avionics.status = { true, 1 };
	TEST_EXPECT(context, exporter.export_snapshot(snapshot, actions).count == 1);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McGeneration), 1.0, kTolerance);
	cockpit.set_available(DcsIds::CockpitParams::McMissileUncagePressSequence, true);
	TEST_EXPECT(context, exporter.export_snapshot(snapshot, actions).count == 1);
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
	Core::MissionActionBatch actions;
	snapshot.combat_avionics.status = { true, 1 };
	auto events = exporter.export_snapshot(snapshot, actions);
	TEST_EXPECT(context, events.count == 68);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McGeneration), 1.0, kTolerance);
	TEST_EXPECT(context,
	            exporter.export_snapshot(snapshot, actions).count == 0); // Repeated failures remain suppressed.
	for (const auto& entry : DcsIds::CockpitParams::Catalog)
		cockpit.set_available(entry.name, true);
	events = exporter.export_snapshot(snapshot, actions);
	TEST_EXPECT(context, events.count == 68);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McGeneration), 6.0, kTolerance);
	for (std::size_t i = 0; i < events.count; ++i)
		TEST_EXPECT(context, events.items[i].type == DcsBridge::Internal::CockpitParameterEventType::Recovery);
}

void test_gun_count_availability_is_exported_separately(Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	DcsBridge::Internal::CockpitSnapshotExporter exporter(cockpit.api());
	Core::CockpitSnapshot snapshot;
	Core::MissionActionBatch actions;
	snapshot.combat_avionics.status = { true, 1 };
	snapshot.combat_avionics.stores.gun_count = 216;
	exporter.export_snapshot(snapshot, actions);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McStoresGunCount), 216.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McStoresGunCountAvailable), 0.0, kTolerance);
	snapshot.combat_avionics.stores.gun_count = 0;
	snapshot.combat_avionics.stores.gun_ammunition.observation.available = true;
	exporter.export_snapshot(snapshot, actions);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McStoresGunCount), 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McStoresGunCountAvailable), 1.0, kTolerance);
}

void test_selection_mailbox_uses_the_front_command_payload(Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	DcsBridge::Internal::CockpitSnapshotExporter exporter(cockpit.api());
	Core::CockpitSnapshot snapshot;
	Core::MissionActionBatch actions;
	auto& mc = snapshot.combat_avionics;
	mc.status = { true, 1 };
	mc.stores.selection.requested_station = 8;
	mc.stores.selection.request_revision = 99;
	auto& batch = actions;
	batch.first_sequence = 7;
	batch.count = 2;
	batch.actions[0] = { Core::MissionActionKind::SelectStation, 2, 1 };
	batch.actions[1].kind = Core::MissionActionKind::PickleOn;
	exporter.export_snapshot(snapshot, actions);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::WeaponRequestedStation), 2, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::WeaponSelectionRequestRevision), 1, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::WeaponSelectionActionSequence), 7, kTolerance);
	batch.actions[0].kind = Core::MissionActionKind::GunOff;
	batch.actions[1] = { Core::MissionActionKind::SelectStation, 8, 2 };
	exporter.export_snapshot(snapshot, actions);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::WeaponSelectionRequestRevision), 0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::WeaponSelectionActionSequence), 0, kTolerance);
}

void test_ir_observation_validity_clears_unavailable_cockpit_values(Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	DcsBridge::Internal::CockpitSnapshotExporter exporter(cockpit.api());
	Core::CockpitSnapshot snapshot;
	Core::MissionActionBatch actions;
	auto& mc = snapshot.combat_avionics;
	mc.status = { true, 1 };
	auto& ir = mc.stores.ir_seeker;
	ir.status = { true, 2, Core::ObservationInvalidReason::None };
	ir.locked = true;
	ir.desired_azimuth_rad = 0.4;
	exporter.export_snapshot(snapshot, actions);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McStoresIrSeekerLock), 1.0, kTolerance);
	ir.status = { false, 2, Core::ObservationInvalidReason::ParameterUnavailable };
	exporter.export_snapshot(snapshot, actions);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McStoresIrSeekerAvailable), 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McStoresIrSeekerLock), 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McStoresIrDesiredAzimuth), 0.0, kTolerance);
	TEST_EXPECT_NEAR(context,
	                 cockpit.value(DcsIds::CockpitParams::McStoresIrSeekerInvalidReason),
	                 static_cast<double>(Core::ObservationInvalidReason::ParameterUnavailable),
	                 kTolerance);
	ir = {};
	ir.status = { true, 4, Core::ObservationInvalidReason::None };
	exporter.export_snapshot(snapshot, actions);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McStoresIrSeekerAvailable), 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McStoresIrSeekerLock), 0.0, kTolerance);
	cockpit.set_available(DcsIds::CockpitParams::McStoresIrSeekerAvailable, false);
	DcsBridge::Internal::CockpitSnapshotExporter missing_exporter(cockpit.api());
	missing_exporter.export_snapshot(snapshot, actions);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McGeneration), 1.0, kTolerance);
	mc.status.available = false;
	missing_exporter.export_snapshot(snapshot, actions);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::McAvailable), 0.0, kTolerance);
}

void test_targeting_submodes_preserve_cockpit_wire_values(Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	DcsBridge::Internal::CockpitSnapshotExporter exporter(cockpit.api());
	Core::CockpitSnapshot snapshot;
	snapshot.combat_avionics.status = { true, 1 };
	snapshot.combat_avionics.fire_control.air_to_air_submode = Core::TargetAcquisitionMode::VerticalScan;
	exporter.export_snapshot(snapshot);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::HmcsAamSubmode), 2.0, kTolerance);
	snapshot.combat_avionics.fire_control.air_to_air_submode = Core::TargetAcquisitionMode::Hud;
	exporter.export_snapshot(snapshot);
	TEST_EXPECT_NEAR(context, cockpit.value(DcsIds::CockpitParams::HmcsAamSubmode), 3.0, kTolerance);
}

}

void run_cockpit_snapshot_exporter_tests(Tests::Context& context)
{
	test_targeting_submodes_preserve_cockpit_wire_values(context);
	test_ir_observation_validity_clears_unavailable_cockpit_values(context);
	test_selection_mailbox_uses_the_front_command_payload(context);
	test_gun_count_availability_is_exported_separately(context);
	test_snapshot_envelope_is_exported(context);
	test_missing_parameter_reports_once_then_recovers(context);
	test_reset_restarts_error_reporting(context);
	test_mc_window_commits_only_when_all_parameters_are_written(context);
	test_missing_control_fact_blocks_mc_commit(context);
	test_exporter_reports_all_missing_fields_and_recoveries(context);
}
