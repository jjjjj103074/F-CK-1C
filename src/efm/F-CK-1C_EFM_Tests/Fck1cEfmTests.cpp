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

	void on_first_frame(const Core::Fck1cEfmSnapshot&) override { ++first_frames; }
	void on_engine_shutdown(const Core::Fck1cEfmSnapshot&) override { ++engine_shutdowns; }
	void on_thrust_updated(
		const Core::Fck1cEfmSnapshot&,
		const Core::MaxPowerCommand&) override { ++thrust_updates; }
	void on_ground_diagnostics(
		const Core::Fck1cEfmSnapshot&,
		double) override { ++ground_updates; }
	void on_release(const Core::Fck1cEfmSnapshot& snapshot) override
	{
		++release_notifications;
		release_damage_integrity = snapshot.systems.damage.left_engine_integrity;
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

void send_command(
	Core::Fck1cEfm& efm,
	const Core::EfmCommand& command)
{
	efm.handle_command(command);
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
	bool rejected = false;
	try
	{
		Core::Fck1cEfm efm(Data::AircraftConfig(), runtime);
	}
	catch (const std::invalid_argument&)
	{
		rejected = true;
	}
	TEST_EXPECT(context, rejected);
}

void test_snapshot_isolation(Tests::Context& context)
{
	TestRuntime runtime;
	Core::Fck1cEfm efm(make_test_config(), runtime);
	efm.set_mass_state({ 9100.0, Common::Vec3(1.0, 2.0, 3.0) });
	efm.set_internal_fuel(1200.0);
	efm.set_easy_flight(true);
	Core::Fck1cEfmSnapshot copy = efm.snapshot();
	copy.aircraft.current_mass = 1.0;
	copy.systems.fuel.internal_fuel = 2.0;
	copy.gameplay.easy_flight = false;
	const Core::Fck1cEfmSnapshot current = efm.snapshot();
	TEST_EXPECT_NEAR(context, current.aircraft.current_mass, 9100.0, kTolerance);
	TEST_EXPECT_NEAR(context, current.systems.fuel.internal_fuel, 1200.0, kTolerance);
	TEST_EXPECT(context, current.gameplay.easy_flight);
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
	efm.set_internal_fuel(100.0);
	efm.simulate(0.01);
	Core::Fck1cEfmSnapshot state = efm.snapshot();
	TEST_EXPECT(context, runtime.first_frames == 1);
	TEST_EXPECT(context, runtime.autopilot_reads == 1);
	TEST_EXPECT(context, runtime.max_power_reads == 1);
	TEST_EXPECT(context, runtime.thrust_updates == 1);
	TEST_EXPECT(context, runtime.ground_updates == 1);
	TEST_EXPECT(context, runtime.engine_shutdowns == 0);
	TEST_EXPECT(context, state.systems.startup.first_frame_completed);
	TEST_EXPECT_NEAR(context, state.systems.startup.simulation_time, 0.01, kTolerance);
	TEST_EXPECT_NEAR(context, state.systems.primary_controls.pitch.input, 0.2, kTolerance);
	TEST_EXPECT_NEAR(context, state.systems.primary_controls.roll.input, -0.3, kTolerance);
	efm.simulate(0.01);
	state = efm.snapshot();
	TEST_EXPECT(context, runtime.first_frames == 1);
	TEST_EXPECT(context, runtime.thrust_updates == 2);
	TEST_EXPECT(context, runtime.ground_updates == 2);
	TEST_EXPECT_NEAR(context, state.systems.startup.simulation_time, 0.02, kTolerance);
}

void test_ground_start_lifecycle(Tests::Context& context)
{
	TestRuntime runtime;
	Core::Fck1cEfm efm(make_test_config(), runtime);
	efm.apply_damage({ Core::DamageArea::LeftWing, 0, 0.2 });
	efm.cold_start();
	Core::Fck1cEfmSnapshot state = efm.snapshot();
	TEST_EXPECT(context, state.systems.startup.mode == Systems::STARTUP_MODE_COLD_GROUND);
	TEST_EXPECT_NEAR(context, state.systems.damage.left_wing_integrity, 1.0, kTolerance);
	TEST_EXPECT(context, state.systems.landing_gear.switch_down);
	TEST_EXPECT_NEAR(context, state.systems.landing_gear.position, 1.0, kTolerance);
	TEST_EXPECT(context, !state.systems.engines.left.switch_on);
	efm.hot_ground_start();
	state = efm.snapshot();
	TEST_EXPECT(context, state.systems.startup.mode == Systems::STARTUP_MODE_HOT_GROUND);
	TEST_EXPECT(context, state.systems.airframe_devices.flap_mode == Systems::FLAP_MODE_DOWN);
	TEST_EXPECT_NEAR(context, state.systems.airframe_devices.flaps_pos, 1.0, kTolerance);
	TEST_EXPECT(context, state.systems.engines.left.switch_on);
	TEST_EXPECT_NEAR(context, state.systems.engines.left.throttle_output, 0.5, kTolerance);
}

void test_air_start_lifecycle(Tests::Context& context)
{
	TestRuntime runtime;
	Core::Fck1cEfm efm(make_test_config(), runtime);
	efm.hot_air_start();
	const Core::Fck1cEfmSystems systems = efm.snapshot().systems;
	TEST_EXPECT(context, systems.startup.mode == Systems::STARTUP_MODE_HOT_AIR);
	TEST_EXPECT(context, !systems.landing_gear.switch_down);
	TEST_EXPECT_NEAR(context, systems.landing_gear.position, 0.0, kTolerance);
	TEST_EXPECT(context, !systems.landing_gear.wheels.nose_turn_enabled);
	TEST_EXPECT_NEAR(context, systems.throttle_inputs.left.pilot_cmd, 0.5, kTolerance);
	TEST_EXPECT(context, systems.engines.left.switch_on);
	TEST_EXPECT_NEAR(context, systems.engines.left.throttle_input, 0.5, kTolerance);
}

void test_release_lifecycle(Tests::Context& context)
{
	TestRuntime runtime;
	Core::Fck1cEfm efm(make_test_config(), runtime);
	efm.hot_ground_start();
	efm.simulate(0.01);
	efm.apply_damage({ Core::DamageArea::LeftEngine, 0, 0.2 });
	send_command(efm,
		{ Core::CommandGroup::PitchRoll, Core::CommandAction::SetPitchAxis, 0.5 });
	send_command(efm,
		{ Core::CommandGroup::LandingGear, Core::CommandAction::SetLeftBrake, 1.0 });
	efm.release();
	const Core::Fck1cEfmSnapshot state = efm.snapshot();
	TEST_EXPECT(context, state.systems.startup.mode == Systems::STARTUP_MODE_RELEASED);
	TEST_EXPECT_NEAR(context, state.systems.startup.simulation_time, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, state.systems.primary_controls.pitch.input, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, state.control_surfaces.elevator_command, 0.0, kTolerance);
	TEST_EXPECT(context, !state.systems.landing_gear.wheels.nose_turn_enabled);
	TEST_EXPECT_NEAR(context, state.systems.landing_gear.wheels.brake_left, 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, state.systems.engines.left.nozzle_aperture, 0.8, kTolerance);
	TEST_EXPECT(context, runtime.release_notifications == 1);
	TEST_EXPECT_NEAR(context, runtime.release_damage_integrity, 0.2, kTolerance);
	TEST_EXPECT_NEAR(context, state.systems.damage.left_engine_integrity, 1.0, kTolerance);
}
}

void run_fck1c_efm_tests(Tests::Context& context)
{
	test_config_ownership(context);
	test_invalid_config_rejected(context);
	test_snapshot_isolation(context);
	test_simulation_pipeline(context);
	test_ground_start_lifecycle(context);
	test_air_start_lifecycle(context);
	test_release_lifecycle(context);
}
