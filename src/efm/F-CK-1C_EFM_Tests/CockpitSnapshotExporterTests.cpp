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
	test_missing_parameter_reports_once_then_recovers(context);
	test_reset_restarts_error_reporting(context);
}
