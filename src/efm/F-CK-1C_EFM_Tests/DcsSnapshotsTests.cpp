#include "TestHarness.h"

#include "DcsBridge/DcsSnapshots.h"

#include <cstring>

namespace
{
constexpr double kTolerance = 1e-9;

class NullRuntime final : public Core::Fck1cEfmRuntime
{
public:
	Core::AutopilotCommand read_autopilot() override { return {}; }
	Core::MaxPowerCommand read_max_power() override { return {}; }
	void on_first_frame(const Core::Fck1cEfm&) override {}
	void on_engine_shutdown(const Core::Fck1cEfm&) override {}
	void on_thrust_updated(const Core::Fck1cEfm&, const Core::MaxPowerCommand&) override {}
	void on_ground_diagnostics(const Core::Fck1cEfm&, double) override {}
	void on_release(const Core::Fck1cEfm&) override {}
};

void seed_snapshot_state(Core::Fck1cEfm& efm)
{
	Core::Fck1cEfmSystems& systems = efm.systems();
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
	efm.control_surfaces().elevator_command = 0.2;
	efm.control_surfaces().aileron_command = -0.1;
	efm.control_surfaces().rudder_command = 0.3;
	efm.aircraft_state().atmosphere_temperature = 288.0;
}

void test_draw_arg_snapshot(Tests::Context& context)
{
	NullRuntime runtime;
	Core::Fck1cEfm efm(Core::Fck1cEfmConfig(), runtime);
	seed_snapshot_state(efm);
	const DcsBridge::DrawArgState state = DcsBridge::make_draw_arg_state(efm);
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
	NullRuntime runtime;
	Core::Fck1cEfm efm(Core::Fck1cEfmConfig(), runtime);
	seed_snapshot_state(efm);
	const DcsBridge::ParamExportState state = DcsBridge::make_param_export_state(efm);
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
	NullRuntime runtime;
	Core::Fck1cEfm efm(Core::Fck1cEfmConfig(), runtime);
	efm.aircraft_state().altitude_asl = 1200.0;
	efm.systems().landing_gear.position = 1.0;
	efm.systems().suspension.wow[1] = true;
	const Diagnostics::DebugWatchSnapshot state =
		DcsBridge::make_debug_watch_snapshot(efm, "test", "date");
	TEST_EXPECT(context, std::strcmp(state.version, "test") == 0);
	TEST_EXPECT_NEAR(context, state.altitude_asl, 1200.0, kTolerance);
	TEST_EXPECT_NEAR(context, state.gear_pos, 1.0, kTolerance);
	TEST_EXPECT(context, state.wow[1]);
	TEST_EXPECT(context, state.fbw == &efm.systems().fbw);
}
}

void run_dcs_snapshots_tests(Tests::Context& context)
{
	test_draw_arg_snapshot(context);
	test_param_snapshot(context);
	test_debug_watch_snapshot(context);
}
