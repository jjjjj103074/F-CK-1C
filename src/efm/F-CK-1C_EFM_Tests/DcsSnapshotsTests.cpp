#include "TestHarness.h"

#include "DcsBridge/DcsSnapshots.h"

#include <cstring>

namespace
{
constexpr double kTolerance = 1e-9;

Core::Fck1cEfmSnapshot make_snapshot()
{
	Core::Fck1cEfmSnapshot snapshot;
	Core::Fck1cEfmSystems& systems = snapshot.systems;
	systems.landing_gear.position = 0.8;
	systems.landing_gear.wheels.nose_steering = -0.25;
	systems.landing_gear.wheels.spin[0] = 1.0;
	systems.landing_gear.wheels.spin[1] = 2.0;
	systems.landing_gear.wheels.spin[2] = 3.0;
	systems.airframe_devices.flaps_pos = 0.4;
	systems.airframe_devices.airbrake_pos = 0.6;
	systems.engines.left.switch_on = true;
	systems.engines.left.throttle_input = 0.7;
	systems.engines.left.thrust_force = 12000.0;
	systems.fuel.internal_fuel = 900.0;
	systems.fuel.total_fuel = 1100.0;
	snapshot.control_surfaces.elevator_command = 0.2;
	snapshot.control_surfaces.aileron_command = -0.1;
	snapshot.control_surfaces.rudder_command = 0.3;
	snapshot.aircraft.atmosphere_temperature = 288.0;
	return snapshot;
}

void test_draw_arg_snapshot(Tests::Context& context)
{
	const DcsBridge::DrawArgState state =
		DcsBridge::make_draw_arg_state(make_snapshot());
	TEST_EXPECT_NEAR(context, state.gear_pos, 0.8, kTolerance);
	TEST_EXPECT_NEAR(context, state.nose_wheel_steering, -0.25, kTolerance);
	TEST_EXPECT_NEAR(context, state.elevator_command, 0.2, kTolerance);
	TEST_EXPECT_NEAR(context, state.flaps_pos, 0.4, kTolerance);
	TEST_EXPECT_NEAR(context, state.aileron_command, -0.1, kTolerance);
	TEST_EXPECT_NEAR(context, state.rudder_command, 0.3, kTolerance);
	TEST_EXPECT_NEAR(context, state.wheel_spin[2], 3.0, kTolerance);
}

void test_param_snapshot(Tests::Context& context)
{
	const DcsBridge::ParamExportState state =
		DcsBridge::make_param_export_state(make_snapshot());
	TEST_EXPECT_NEAR(context, state.gear_pos, 0.8, kTolerance);
	TEST_EXPECT(context, state.left_engine_switch);
	TEST_EXPECT_NEAR(context, state.left_throttle_input, 0.7, kTolerance);
	TEST_EXPECT_NEAR(context, state.left_thrust_force, 12000.0, kTolerance);
	TEST_EXPECT_NEAR(context, state.atmosphere_temperature, 288.0, kTolerance);
	TEST_EXPECT_NEAR(context, state.internal_fuel, 900.0, kTolerance);
	TEST_EXPECT_NEAR(context, state.total_fuel, 1100.0, kTolerance);
}

void test_debug_watch_snapshot(Tests::Context& context)
{
	Core::Fck1cEfmSnapshot source = make_snapshot();
	source.aircraft.altitude_asl = 1200.0;
	source.systems.landing_gear.position = 1.0;
	source.systems.suspension.wow[1] = true;
	const Diagnostics::DebugWatchSnapshot state =
		DcsBridge::make_debug_watch_snapshot(source, "test", "date");
	TEST_EXPECT(context, std::strcmp(state.version, "test") == 0);
	TEST_EXPECT_NEAR(context, state.altitude_asl, 1200.0, kTolerance);
	TEST_EXPECT_NEAR(context, state.gear_pos, 1.0, kTolerance);
	TEST_EXPECT(context, state.wow[1]);
	TEST_EXPECT(context, state.fbw.mode_target == source.systems.fbw.mode_target);
}

void test_debug_watch_formatting(Tests::Context& context)
{
	Diagnostics::DebugWatchSnapshot snapshot;
	snapshot.version = "test";
	snapshot.version_date = "date";
	char compact[2048];
	char detailed[4096];
	const size_t compact_size = Diagnostics::format_debug_watch(
		0, snapshot, { compact, sizeof(compact) });
	const size_t detailed_size = Diagnostics::format_debug_watch(
		1, snapshot, { detailed, sizeof(detailed) });
	TEST_EXPECT(context, compact_size > 0);
	TEST_EXPECT(context, detailed_size > compact_size);
	TEST_EXPECT(context, std::strstr(compact, "VER:test DATE:date") != nullptr);
	TEST_EXPECT(context, std::strstr(detailed, "ACT_E:") != nullptr);
}
}

void run_dcs_snapshots_tests(Tests::Context& context)
{
	test_draw_arg_snapshot(context);
	test_param_snapshot(context);
	test_debug_watch_snapshot(context);
	test_debug_watch_formatting(context);
}
