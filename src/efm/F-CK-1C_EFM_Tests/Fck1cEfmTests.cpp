#include "TestHarness.h"

#include "Core/Fck1cEfm.h"

#include <stdexcept>

namespace
{
constexpr double kTolerance = 1e-9;

class TestRuntime final : public Core::Fck1cEfmRuntime
{
public:
	Core::AutopilotCommand read_autopilot() override
	{
		++autopilot_reads;
		return autopilot;
	}

	Core::MaxPowerCommand read_max_power() override
	{
		++max_power_reads;
		return max_power;
	}

	void on_first_frame(const Core::Fck1cEfm&) override
	{
		++first_frames;
	}

	void on_engine_shutdown(const Core::Fck1cEfm&) override
	{
		++engine_shutdowns;
	}

	void on_thrust_updated(const Core::Fck1cEfm&, const Core::MaxPowerCommand&) override
	{
		++thrust_updates;
	}

	void on_ground_diagnostics(const Core::Fck1cEfm&, double) override
	{
		++ground_updates;
	}

	void on_release(const Core::Fck1cEfm& efm) override
	{
		++release_notifications;
		release_damage_integrity = efm.systems().damage.left_engine_integrity;
	}

	Core::AutopilotCommand autopilot;
	Core::MaxPowerCommand max_power;
	int autopilot_reads = 0;
	int max_power_reads = 0;
	int first_frames = 0;
	int engine_shutdowns = 0;
	int thrust_updates = 0;
	int ground_updates = 0;
	int release_notifications = 0;
	double release_damage_integrity = 0.0;
};

Data::AircraftConfig make_test_config()
{
	Data::AircraftConfig config;
	config.aerodynamics.wing_area = 24.26;
	config.aerodynamics.wingspan = 8.53;
	config.aerodynamics.length = 14.48;
	config.aerodynamics.height = 4.7;
	config.aerodynamics.mach_max = 1.5;
	config.aerodynamics.mach_table = { 0.0, 1.0 };
	config.aerodynamics.cx_zero_table = { 0.025, 0.030 };
	config.aerodynamics.cy_alpha_table = { 0.05, 0.04 };
	config.aerodynamics.roll_rate_max_table = { 3.0, 2.0 };
	config.aerodynamics.alpha_max_table = { 20.0, 18.0 };
	config.aerodynamics.cy_max_table = { 1.2, 1.0 };
	config.engine.start_time = 5.0;
	config.engine.spool_up_tau = 1.0;
	config.engine.spool_down_tau = 1.0;
	config.engine.mach_table = { 0.0, 1.0 };
	config.engine.max_thrust_table = { 54000.0, 50000.0 };
	config.engine.throttle_input_table = { 0.0, 1.0 };
	config.engine.power_table = { 0.1, 1.0 };
	config.left_engine_position = Common::Vec3(-3.793, -0.391, -0.716);
	config.right_engine_position = Common::Vec3(-3.793, -0.391, 0.716);
	return config;
}

void test_config_ownership(Tests::Context& context)
{
	TestRuntime runtime;
	Data::AircraftConfig source = make_test_config();
	Core::Fck1cEfm efm(source, runtime);
	source.aerodynamics.mach_table[0] = 99.0;
	source.engine.max_thrust_table[0] = 0.0;
	TEST_EXPECT_NEAR(context, efm.config().aerodynamics.wing_area, 24.26, kTolerance);
	TEST_EXPECT_NEAR(context, efm.config().aerodynamics.mach_table[0], 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, efm.config().engine.max_thrust_table[0], 54000.0, kTolerance);
	TEST_EXPECT_NEAR(context, efm.config().left_engine_position.z, -0.716, kTolerance);
	TEST_EXPECT_NEAR(context, efm.config().right_engine_position.z, 0.716, kTolerance);
}

void test_invalid_config_rejected(Tests::Context& context)
{
	TestRuntime runtime;
	Data::AircraftConfig invalid_config;
	bool rejected = false;
	try
	{
		Core::Fck1cEfm efm(invalid_config, runtime);
	}
	catch (const std::invalid_argument&)
	{
		rejected = true;
	}
	TEST_EXPECT(context, rejected);
}

void test_runtime_state_ownership(Tests::Context& context)
{
	TestRuntime runtime;
	Core::Fck1cEfm efm(make_test_config(), runtime);
	efm.aircraft_state().current_mass = 9100.0;
	efm.systems().fuel.internal_fuel = 1200.0;
	efm.control_surfaces().elevator_command = 0.25;
	efm.gameplay().easy_flight = true;

	const Core::Fck1cEfm& read_only = efm;
	TEST_EXPECT_NEAR(context, read_only.aircraft_state().current_mass, 9100.0, kTolerance);
	TEST_EXPECT_NEAR(context, read_only.systems().fuel.internal_fuel, 1200.0, kTolerance);
	TEST_EXPECT_NEAR(context, read_only.control_surfaces().elevator_command, 0.25, kTolerance);
	TEST_EXPECT(context, read_only.gameplay().easy_flight);
}

void test_simulation_pipeline(Tests::Context& context)
{
	TestRuntime runtime;
	runtime.autopilot.master = true;
	runtime.autopilot.pitch_command = 0.2;
	runtime.autopilot.roll_command = -0.3;
	runtime.autopilot.auto_throttle_engaged = true;
	runtime.autopilot.throttle_command = 0.4;
	Core::Fck1cEfm efm(make_test_config(), runtime);
	efm.systems().fuel.internal_fuel = 100.0;

	efm.simulate(0.01);

	TEST_EXPECT(context, runtime.first_frames == 1);
	TEST_EXPECT(context, runtime.autopilot_reads == 1);
	TEST_EXPECT(context, runtime.max_power_reads == 1);
	TEST_EXPECT(context, runtime.thrust_updates == 1);
	TEST_EXPECT(context, runtime.ground_updates == 1);
	TEST_EXPECT(context, runtime.engine_shutdowns == 0);
	TEST_EXPECT(context, efm.systems().startup.first_frame_completed);
	TEST_EXPECT_NEAR(context, efm.systems().startup.simulation_time, 0.01, kTolerance);
	TEST_EXPECT_NEAR(context, efm.systems().primary_controls.pitch.input, 0.2, kTolerance);
	TEST_EXPECT_NEAR(context, efm.systems().primary_controls.roll.input, -0.3, kTolerance);

	efm.simulate(0.01);
	TEST_EXPECT(context, runtime.first_frames == 1);
	TEST_EXPECT(context, runtime.thrust_updates == 2);
	TEST_EXPECT(context, runtime.ground_updates == 2);
	TEST_EXPECT_NEAR(context, efm.systems().startup.simulation_time, 0.02, kTolerance);
}

void test_ground_start_lifecycle(Tests::Context& context)
{
	TestRuntime runtime;
	Core::Fck1cEfm efm(make_test_config(), runtime);
	efm.systems().damage.left_wing_integrity = 0.2;
	efm.cold_start();
	TEST_EXPECT(context, efm.systems().startup.mode == Systems::STARTUP_MODE_COLD_GROUND);
	TEST_EXPECT_NEAR(context, efm.systems().damage.left_wing_integrity, 1.0, kTolerance);
	TEST_EXPECT(context, efm.systems().landing_gear.switch_down);
	TEST_EXPECT_NEAR(context, efm.systems().landing_gear.position, 1.0, kTolerance);
	TEST_EXPECT(context, !efm.systems().engines.left.switch_on);

	efm.hot_ground_start();
	TEST_EXPECT(context, efm.systems().startup.mode == Systems::STARTUP_MODE_HOT_GROUND);
	TEST_EXPECT(context, efm.systems().airframe_devices.flap_mode == Systems::FLAP_MODE_DOWN);
	TEST_EXPECT_NEAR(context, efm.systems().airframe_devices.flaps_pos, 1.0, kTolerance);
	TEST_EXPECT(context, efm.systems().engines.left.switch_on);
	TEST_EXPECT_NEAR(context, efm.systems().engines.left.throttle_output, 0.5, kTolerance);
}

void test_air_start_lifecycle(Tests::Context& context)
{
	TestRuntime runtime;
	Core::Fck1cEfm efm(make_test_config(), runtime);
	efm.hot_air_start();
	TEST_EXPECT(context, efm.systems().startup.mode == Systems::STARTUP_MODE_HOT_AIR);
	TEST_EXPECT(context, !efm.systems().landing_gear.switch_down);
	TEST_EXPECT_NEAR(context, efm.systems().landing_gear.position, 0.0, kTolerance);
	TEST_EXPECT(context, !efm.systems().landing_gear.wheels.nose_turn_enabled);
	TEST_EXPECT_NEAR(context, efm.systems().throttle_inputs.left.pilot_cmd, 0.5, kTolerance);
	TEST_EXPECT(context, efm.systems().engines.left.switch_on);
	TEST_EXPECT_NEAR(context, efm.systems().engines.left.throttle_input, 0.5, kTolerance);
}

void test_release_lifecycle(Tests::Context& context)
{
	TestRuntime runtime;
	Core::Fck1cEfm efm(make_test_config(), runtime);
	efm.hot_ground_start();
	efm.systems().startup.simulation_time = 4.0;
	efm.systems().primary_controls.pitch.input = 0.5;
	efm.control_surfaces().elevator_command = 0.4;
	efm.systems().damage.left_engine_integrity = 0.2;
	efm.systems().landing_gear.wheels.brake_left = 0.7;
	efm.systems().landing_gear.wheels.spin[0] = 0.6;
	efm.release();
	TEST_EXPECT(context, efm.systems().startup.mode == Systems::STARTUP_MODE_RELEASED);
	TEST_EXPECT_NEAR(context, efm.systems().startup.simulation_time, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, efm.systems().primary_controls.pitch.input, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, efm.control_surfaces().elevator_command, 0.0, kTolerance);
	TEST_EXPECT(context, !efm.systems().landing_gear.wheels.nose_turn_enabled);
	TEST_EXPECT_NEAR(context, efm.systems().landing_gear.wheels.brake_left, 0.7, kTolerance);
	TEST_EXPECT_NEAR(context, efm.systems().landing_gear.wheels.spin[0], 0.6, kTolerance);
	TEST_EXPECT_NEAR(context, efm.systems().engines.left.nozzle_aperture, 0.8, kTolerance);
	TEST_EXPECT(context, runtime.release_notifications == 1);
	TEST_EXPECT_NEAR(context, runtime.release_damage_integrity, 0.2, kTolerance);
	TEST_EXPECT_NEAR(context, efm.systems().damage.left_engine_integrity, 1.0, kTolerance);
}
}

void run_fck1c_efm_tests(Tests::Context& context)
{
	test_config_ownership(context);
	test_invalid_config_rejected(context);
	test_runtime_state_ownership(context);
	test_simulation_pipeline(context);
	test_ground_start_lifecycle(context);
	test_air_start_lifecycle(context);
	test_release_lifecycle(context);
}
