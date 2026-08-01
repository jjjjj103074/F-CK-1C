#include "FakeCockpitParameters.h"
#include "TestHarness.h"

#include "DcsBridge/Internal/CockpitSnapshotExporter.h"
#include "DcsIds/CockpitParams.g.h"

#include <cstring>

namespace
{
constexpr double kTolerance = 1e-9;
constexpr double kMetersPerFoot = 0.3048;
constexpr double kMetersPerSecondPerKnot = 0.5144444444444445;

void test_snapshot_envelope_is_exported(Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	DcsBridge::Internal::CockpitSnapshotExporter exporter(cockpit.api());
	Core::CockpitSnapshot snapshot;
	snapshot.status = { true, 42 };
	snapshot.simulation_time_s = 12.5;
	snapshot.propulsion_test_thrust_cut_requested = true;
	const DcsBridge::Internal::CockpitParameterEvents events =
		exporter.export_snapshot(snapshot);
	TEST_EXPECT(context, events.count == 0);
	TEST_EXPECT_NEAR(
		context,
		cockpit.value(DcsIds::CockpitParams::CockpitSnapshotAvailable),
		1.0,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		cockpit.value(DcsIds::CockpitParams::CockpitSnapshotRevision),
		42.0,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		cockpit.value(DcsIds::CockpitParams::CockpitSnapshotTimeS),
		12.5,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		cockpit.value(DcsIds::CockpitParams::MaxPowerSwitch),
		0.0,
		kTolerance);
}

void test_automatic_flight_control_snapshot_is_exported(
	Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	DcsBridge::Internal::CockpitSnapshotExporter exporter(cockpit.api());
	Core::CockpitSnapshot snapshot;
	auto& afcs = snapshot.automatic_flight_control;
	afcs.master_engaged = true;
	afcs.bypass_active = true;
	afcs.auto_throttle_engaged = true;
	afcs.vertical_mode =
		Core::AutomaticFlightControlVerticalMode::AltitudeHold;
	afcs.lateral_mode =
		Core::AutomaticFlightControlLateralMode::HeadingSelect;
	afcs.pitch_attitude_reference_rad = 0.2;
	afcs.vertical_speed_reference_mps = 3.0;
	afcs.bank_angle_reference_rad = -0.1;
	afcs.target_altitude_m = 1000.0 * kMetersPerFoot;
	afcs.target_speed_mps = 300.0 * kMetersPerSecondPerKnot;
	afcs.autopilot_disengage_reason =
		Core::AutomaticFlightControlReason::WeightOnWheels;
	TEST_EXPECT(context, exporter.export_snapshot(snapshot).count == 0);
	TEST_EXPECT_NEAR(context, cockpit.value(
		DcsIds::CockpitParams::ApMasterEngaged), 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(
		DcsIds::CockpitParams::ApVerticalMode), 3.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(
		DcsIds::CockpitParams::ApLateralMode), 2.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(
		DcsIds::CockpitParams::ApPitchAttitudeReference), 0.2, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(
		DcsIds::CockpitParams::ApVerticalSpeedReference), 3.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(
		DcsIds::CockpitParams::ApBankAngleReference), -0.1, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(
		DcsIds::CockpitParams::ApTargetAltitudeFt), 1000.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(
		DcsIds::CockpitParams::ApTargetSpeedKts), 300.0, kTolerance);
	TEST_EXPECT_NEAR(context, cockpit.value(
		DcsIds::CockpitParams::ApDisengageReason), 3.0, kTolerance);
}

void test_missing_parameter_reports_once_then_recovers(
	Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	cockpit.set_available(
		DcsIds::CockpitParams::CockpitSnapshotRevision,
		false);
	DcsBridge::Internal::CockpitSnapshotExporter exporter(cockpit.api());
	Core::CockpitSnapshot snapshot;
	snapshot.status = { true, 3 };
	DcsBridge::Internal::CockpitParameterEvents events =
		exporter.export_snapshot(snapshot);
	TEST_EXPECT(context, events.count == 1);
	TEST_EXPECT(context, std::strcmp(
		events.items[0].parameter_name,
		DcsIds::CockpitParams::CockpitSnapshotRevision) == 0);
	TEST_EXPECT(
		context,
		events.items[0].type ==
			DcsBridge::Internal::CockpitParameterEventType::Error);
	TEST_EXPECT(context, exporter.export_snapshot(snapshot).count == 0);
	cockpit.set_available(
		DcsIds::CockpitParams::CockpitSnapshotRevision,
		true);
	events = exporter.export_snapshot(snapshot);
	TEST_EXPECT(context, events.count == 1);
	TEST_EXPECT(
		context,
		events.items[0].type ==
			DcsBridge::Internal::CockpitParameterEventType::Recovery);
}

void test_reset_restarts_error_reporting(Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	cockpit.set_available(
		DcsIds::CockpitParams::CockpitSnapshotAvailable,
		false);
	DcsBridge::Internal::CockpitSnapshotExporter exporter(cockpit.api());
	TEST_EXPECT(context, exporter.export_snapshot({}).count == 1);
	TEST_EXPECT(context, exporter.export_snapshot({}).count == 0);
	exporter.reset();
	TEST_EXPECT(context, exporter.export_snapshot({}).count == 1);
}
}

void run_cockpit_snapshot_exporter_tests(Tests::Context& context)
{
	test_snapshot_envelope_is_exported(context);
	test_automatic_flight_control_snapshot_is_exported(context);
	test_missing_parameter_reports_once_then_recovers(context);
	test_reset_restarts_error_reporting(context);
}
