#include "TestHarness.h"
#include "Fck1cEfmTestFixture.h"

#include "Core/Fck1cEfm.h"

#include <limits>
#include <stdexcept>

namespace
{
constexpr double kTolerance = 1e-9;
constexpr double kSimulationStepS = 0.01;

using Tests::Fck1c::all_frame_data_available;
using Tests::Fck1c::make_frame_input;
using Tests::Fck1c::make_test_config;

void expect_vec3(
	Tests::Context& context,
	const Common::Vec3& actual,
	const Common::Vec3& expected)
{
	TEST_EXPECT_NEAR(context, actual.x, expected.x, kTolerance);
	TEST_EXPECT_NEAR(context, actual.y, expected.y, kTolerance);
	TEST_EXPECT_NEAR(context, actual.z, expected.z, kTolerance);
}

void expect_availability(
	Tests::Context& context,
	const Core::FrameDataAvailability& actual,
	const Core::FrameDataAvailability& expected)
{
	TEST_EXPECT(context, actual.atmosphere == expected.atmosphere);
	TEST_EXPECT(context, actual.surface == expected.surface);
	TEST_EXPECT(context, actual.mass == expected.mass);
	TEST_EXPECT(context, actual.world_kinematics == expected.world_kinematics);
	TEST_EXPECT(context, actual.body_kinematics == expected.body_kinematics);
	for (std::size_t index = 0; index < actual.suspension.size(); ++index)
	{
		TEST_EXPECT(context, actual.suspension[index] == expected.suspension[index]);
	}
}

void expect_start_output(
	Tests::Context& context,
	const Core::FrameOutput& output,
	Core::StartMode mode)
{
	const bool hot = mode != Core::StartMode::ColdGround;
	const bool airborne = mode == Core::StartMode::HotAir;
	TEST_EXPECT_NEAR(context, output.simulation_time_s, 0.0, kTolerance);
	TEST_EXPECT(context, output.engines[0].switch_on == hot);
	TEST_EXPECT(context, output.engines[1].switch_on == hot);
	TEST_EXPECT_NEAR(
		context, output.landing_gear.gear_position, airborne ? 0.0 : 1.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, output.engines[0].throttle_input, airborne ? 0.5 : 0.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, output.engines[1].throttle_output, hot ? 0.5 : 0.0, kTolerance);
}

void expect_engine_baseline(
	Tests::Context& context,
	const Core::EngineOutput& actual)
{
	TEST_EXPECT(context, actual.switch_on);
	TEST_EXPECT_NEAR(context, actual.throttle_input, 0.4, kTolerance);
	TEST_EXPECT_NEAR(context, actual.throttle_output, 0.50224089635854341, kTolerance);
	TEST_EXPECT_NEAR(context, actual.power_readout, 0.50150000000000006, kTolerance);
	TEST_EXPECT_NEAR(context, actual.thrust_force, 12959.196801553151, kTolerance);
	TEST_EXPECT_NEAR(context, actual.afterburner_ratio, 0.0, kTolerance);
	TEST_EXPECT(context, !actual.afterburner_lit);
	TEST_EXPECT_NEAR(context, actual.nozzle_aperture, 0.39300000000000002, kTolerance);
}

void expect_control_baseline(
	Tests::Context& context,
	const Core::ControlOutput& actual)
{
	TEST_EXPECT_NEAR(context, actual.pitch_input, 0.2, kTolerance);
	TEST_EXPECT_NEAR(context, actual.roll_input, -0.3, kTolerance);
	TEST_EXPECT_NEAR(context, actual.yaw_input, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, actual.elevator_command, -0.0037173599739788294, kTolerance);
	TEST_EXPECT_NEAR(context, actual.aileron_command, -0.028571428571428571, kTolerance);
	TEST_EXPECT_NEAR(context, actual.rudder_command, -0.0024916666666666659, kTolerance);
	TEST_EXPECT_NEAR(context, actual.flaps_position, 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, actual.slats_position, 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, actual.airbrake_position, 0.0, kTolerance);
}

void expect_suspension_baseline(
	Tests::Context& context,
	const Core::SuspensionOutput& actual)
{
	const Common::Vec3 forces[] = {
		{ 3.0, 4.0, 0.0 }, { 0.0, 80.0, 0.0 }, { 0.0, 90.0, 0.0 }
	};
	const double compression[] = { 0.1, 0.2, 0.3 };
	const double force_magnitude[] = { 5.0, 80.0, 90.0 };
	for (std::size_t index = 0; index < actual.wheels.size(); ++index)
	{
		expect_vec3(context, actual.wheels[index].acting_force, forces[index]);
		TEST_EXPECT_NEAR(
			context, actual.wheels[index].compression, compression[index], kTolerance);
		TEST_EXPECT_NEAR(
			context, actual.wheels[index].force_magnitude, force_magnitude[index], kTolerance);
		TEST_EXPECT(context, actual.wheels[index].weight_on_wheel);
	}
	TEST_EXPECT(context, actual.any_weight_on_wheels);
	TEST_EXPECT(context, actual.on_ground);
}

void expect_golden_frame(
	Tests::Context& context,
	const Core::FrameOutput& actual)
{
	expect_vec3(context, actual.force_moment.force,
		{ 18282.204829946411, 107068.92218657726, 7465.3009043063385 });
	expect_vec3(context, actual.force_moment.moment,
		{ 26922.073608154937, -2421.6993259174328, -43276.793392220876 });
	expect_vec3(context, actual.force_moment.center_of_mass, { 0.2, -0.1, 0.3 });
	expect_engine_baseline(context, actual.engines[0]);
	expect_engine_baseline(context, actual.engines[1]);
	TEST_EXPECT_NEAR(context, actual.fuel.internal_fuel, 500.0, kTolerance);
	TEST_EXPECT_NEAR(context, actual.fuel.external_fuel, 120.0, kTolerance);
	TEST_EXPECT_NEAR(context, actual.fuel.total_fuel, 620.0, kTolerance);
	TEST_EXPECT_NEAR(context, actual.fuel.total_fuel_flow, 0.0, kTolerance);
	expect_control_baseline(context, actual.controls);
	TEST_EXPECT_NEAR(context, actual.landing_gear.brake_left, 0.4, kTolerance);
	TEST_EXPECT_NEAR(context, actual.landing_gear.brake_right, 0.6, kTolerance);
	expect_suspension_baseline(context, actual.suspension);
}

void test_complete_frame_input_contract(Tests::Context& context)
{
	const Core::FrameInput input = make_frame_input();
	TEST_EXPECT_NEAR(context, input.dt_s, 0.02, kTolerance);
	TEST_EXPECT_NEAR(context, input.atmosphere.pressure, 88000.0, kTolerance);
	TEST_EXPECT_NEAR(context, input.mass.moment_of_inertia.z, 13.0, kTolerance);
	TEST_EXPECT_NEAR(context, input.world_kinematics.position.z, 1200.0, kTolerance);
	TEST_EXPECT_NEAR(context, input.body_kinematics.wind_velocity.x, 4.0, kTolerance);
	TEST_EXPECT_NEAR(context, input.suspension[2].acting_force_point.z, 9.0, kTolerance);
	TEST_EXPECT_NEAR(context, input.suspension[2].integrity_factor, 0.7, kTolerance);
	TEST_EXPECT_NEAR(context, input.suspension[2].wheel_speed_x, 14.0, kTolerance);
	TEST_EXPECT(context, input.autopilot.master);
	TEST_EXPECT_NEAR(context, input.max_power.value, 1.0, kTolerance);
}

void test_frame_dt_contract(Tests::Context& context)
{
	TEST_EXPECT(context, Core::is_valid_frame_dt(0.001));
	TEST_EXPECT(context, !Core::is_valid_frame_dt(0.0));
	TEST_EXPECT(context, !Core::is_valid_frame_dt(-0.001));
	TEST_EXPECT(context, !Core::is_valid_frame_dt(
		std::numeric_limits<double>::quiet_NaN()));
	TEST_EXPECT(context, !Core::is_valid_frame_dt(
		std::numeric_limits<double>::infinity()));
}

void test_all_start_mode_outputs(Tests::Context& context)
{
	const Core::StartMode modes[] = {
		Core::StartMode::ColdGround,
		Core::StartMode::HotGround,
		Core::StartMode::HotAir
	};
	for (Core::StartMode mode : modes)
	{
		Core::Fck1cEfm efm(make_test_config());
		const Core::FrameOutput output = efm.start(mode);
		expect_availability(context, output.availability, {});
		expect_start_output(context, output, mode);
	}
}

void test_frame_output_golden_contract(Tests::Context& context)
{
	Core::Fck1cEfm efm(make_test_config());
	(void)efm.start(Core::StartMode::HotGround);
	efm.set_internal_fuel(500.0);
	efm.set_external_fuel({ 1, 120.0, { 0.5, -0.2, 0.1 } });
	efm.handle_command({
		Core::CommandGroup::LandingGear, Core::CommandAction::SetLeftBrake, 0.4 });
	efm.handle_command({
		Core::CommandGroup::LandingGear, Core::CommandAction::SetRightBrake, 0.6 });
	const Core::FrameInput input = make_frame_input();
	const Core::FrameOutput output = efm.step(input);
	// Golden values captured from 401f44c with this exact translated callback input.
	expect_golden_frame(context, output);
	expect_availability(context, output.availability, input.availability);
	TEST_EXPECT_NEAR(context, output.simulation_time_s, input.dt_s, kTolerance);
}

void test_unavailable_input_preserves_latest_values(Tests::Context& context)
{
	Core::Fck1cEfm efm(make_test_config());
	(void)efm.start(Core::StartMode::HotGround);
	const Core::FrameOutput first = efm.step(make_frame_input());
	Core::FrameInput next_input;
	next_input.dt_s = kSimulationStepS;
	const Core::FrameOutput next = efm.step(next_input);
	expect_availability(context, next.availability, {});
	TEST_EXPECT_NEAR(
		context, next.flight.altitude_asl_m, first.flight.altitude_asl_m, kTolerance);
	expect_vec3(context, next.force_moment.center_of_mass,
		first.force_moment.center_of_mass);
	for (std::size_t index = 0; index < next.suspension.wheels.size(); ++index)
	{
		expect_vec3(context, next.suspension.wheels[index].acting_force,
			first.suspension.wheels[index].acting_force);
		TEST_EXPECT_NEAR(context, next.suspension.wheels[index].compression,
			first.suspension.wheels[index].compression, kTolerance);
	}
}

void test_start_reinitializes_output(Tests::Context& context)
{
	Data::AircraftConfig config = make_test_config();
	config.fuel.consumption_rate = 3.0;
	Core::Fck1cEfm efm(config);
	(void)efm.start(Core::StartMode::HotGround);
	const Core::FrameOutput previous = efm.step(make_frame_input());
	TEST_EXPECT(context, previous.fuel.total_fuel_flow > 0.0);
	const Core::FrameOutput output = efm.start(Core::StartMode::ColdGround);
	TEST_EXPECT_NEAR(context, output.simulation_time_s, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, output.flight.altitude_asl_m, 0.0, kTolerance);
	expect_vec3(context, output.force_moment.force, {});
	expect_vec3(context, output.force_moment.moment, {});
	TEST_EXPECT_NEAR(context, output.controls.flaps_position, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, output.suspension.wheels[0].compression, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, output.engines[0].thrust_force, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, output.fuel.total_fuel_flow, 0.0, kTolerance);
}

void test_release_preparation_survives_start(Tests::Context& context)
{
	Data::AircraftConfig config = make_test_config();
	config.fuel.consumption_rate = 3.0;
	Core::Fck1cEfm efm(config);
	(void)efm.start(Core::StartMode::ColdGround);
	efm.set_internal_fuel(100.0);
	const Core::FrameOutput easy_flight_disabled =
		efm.step(make_frame_input());
	efm.release();
	TEST_EXPECT(context, !efm.take_mass_delta().available);
	efm.set_internal_fuel(200.0);
	efm.set_external_fuel({ 1, 30.0, {} });
	efm.set_infinite_fuel(true);
	efm.set_easy_flight(true);
	efm.set_invincible(true);
	const Core::FrameOutput start = efm.start(Core::StartMode::ColdGround);
	TEST_EXPECT_NEAR(context, start.fuel.internal_fuel, 200.0, kTolerance);
	TEST_EXPECT_NEAR(context, start.fuel.external_fuel, 30.0, kTolerance);
	TEST_EXPECT_NEAR(context, start.fuel.total_fuel, 230.0, kTolerance);
	const Core::DamageEvent damage = { Core::DamageArea::LeftWing, 0, 0.2 };
	TEST_EXPECT(context, efm.apply_damage(damage).invincible);
	const Core::FrameOutput next = efm.step(make_frame_input());
	TEST_EXPECT_NEAR(context, next.fuel.internal_fuel, 200.0, kTolerance);
	TEST_EXPECT_NEAR(context, next.fuel.external_fuel, 30.0, kTolerance);
	TEST_EXPECT(
		context,
		std::fabs(next.force_moment.moment.x -
			easy_flight_disabled.force_moment.moment.x) > kTolerance);
}

void test_config_is_owned_by_core(Tests::Context& context)
{
	Data::AircraftConfig source = make_test_config();
	Core::Fck1cEfm efm(source);
	source.engine.max_thrust_table[0] = 0.0;
	(void)efm.start(Core::StartMode::HotGround);
	efm.set_internal_fuel(100.0);
	Core::FrameInput input;
	input.dt_s = kSimulationStepS;
	const Core::FrameOutput output = efm.step(input);
	TEST_EXPECT(context, output.engines[0].thrust_force > 0.0);
	TEST_EXPECT(context, output.engines[1].thrust_force > 0.0);
}

void test_invalid_config_rejected(Tests::Context& context)
{
	bool rejected = false;
	try
	{
		Core::Fck1cEfm efm(Data::AircraftConfig{});
	}
	catch (const std::invalid_argument&)
	{
		rejected = true;
	}
	TEST_EXPECT(context, rejected);
}

void test_frame_output_isolation(Tests::Context& context)
{
	Core::Fck1cEfm efm(make_test_config());
	(void)efm.start(Core::StartMode::HotGround);
	efm.set_internal_fuel(1200.0);
	Core::FrameInput input;
	input.dt_s = kSimulationStepS;
	Core::FrameOutput copy = efm.step(input);
	copy.fuel.internal_fuel = 2.0;
	copy.controls.pitch_input = 3.0;
	const Core::FrameOutput current = efm.step(input);
	TEST_EXPECT(context, current.fuel.internal_fuel > 1000.0);
	TEST_EXPECT(context, current.controls.pitch_input != 3.0);
}

void test_simulation_pipeline(Tests::Context& context)
{
	Core::Fck1cEfm efm(make_test_config());
	(void)efm.start(Core::StartMode::ColdGround);
	efm.set_internal_fuel(100.0);
	Core::FrameInput input;
	input.dt_s = kSimulationStepS;
	input.autopilot = { true, false, true, 0.2, -0.3, 0.4 };
	const Core::FrameOutput first = efm.step(input);
	TEST_EXPECT_NEAR(context, first.simulation_time_s, kSimulationStepS, kTolerance);
	TEST_EXPECT_NEAR(context, first.controls.pitch_input, 0.2, kTolerance);
	TEST_EXPECT_NEAR(context, first.controls.roll_input, -0.3, kTolerance);
	const Core::FrameOutput second = efm.step(input);
	TEST_EXPECT_NEAR(
		context, second.simulation_time_s, kSimulationStepS * 2.0, kTolerance);
}

void test_cockpit_inputs_drive_outputs(Tests::Context& context)
{
	Core::Fck1cEfm efm(make_test_config());
	(void)efm.start(Core::StartMode::HotGround);
	efm.set_internal_fuel(100.0);
	Core::FrameInput input;
	input.dt_s = kSimulationStepS;
	input.autopilot = { true, false, true, 0.25, -0.35, 0.6 };
	input.max_power = { 1.0, 1.0 };
	Core::FrameOutput output = efm.step(input);
	TEST_EXPECT_NEAR(context, output.controls.pitch_input, 0.25, kTolerance);
	TEST_EXPECT_NEAR(context, output.controls.roll_input, -0.35, kTolerance);
	TEST_EXPECT_NEAR(context, output.engines[0].throttle_input, 0.6, kTolerance);
	TEST_EXPECT(context, output.engines[0].thrust_force > 0.0);
	input.max_power = { 1.0, 0.0 };
	output = efm.step(input);
	TEST_EXPECT_NEAR(context, output.engines[0].thrust_force, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, output.engines[1].thrust_force, 0.0, kTolerance);
}

void test_neutral_cockpit_input_completes_step(Tests::Context& context)
{
	Core::Fck1cEfm efm(make_test_config());
	(void)efm.start(Core::StartMode::HotGround);
	efm.set_internal_fuel(100.0);
	efm.handle_command({
		Core::CommandGroup::PitchRoll, Core::CommandAction::SetPitchAxis, 0.3 });
	Core::FrameInput input;
	input.dt_s = kSimulationStepS;
	const Core::FrameOutput output = efm.step(input);
	TEST_EXPECT_NEAR(context, output.simulation_time_s, kSimulationStepS, kTolerance);
	TEST_EXPECT_NEAR(context, output.controls.pitch_input, 0.3, kTolerance);
	TEST_EXPECT(context, output.engines[0].thrust_force > 0.0);
	TEST_EXPECT(context, output.engines[1].thrust_force > 0.0);
}

void test_damage_returns_immediate_result(Tests::Context& context)
{
	Core::Fck1cEfm efm(make_test_config());
	const Core::DamageEvent damage = { Core::DamageArea::LeftWing, 0, 0.2 };
	TEST_EXPECT(context, !efm.apply_damage(damage).invincible);
	efm.set_invincible(true);
	TEST_EXPECT(context, efm.apply_damage(damage).invincible);
}
}

void run_fck1c_efm_tests(Tests::Context& context)
{
	test_complete_frame_input_contract(context);
	test_frame_dt_contract(context);
	test_all_start_mode_outputs(context);
	test_frame_output_golden_contract(context);
	test_unavailable_input_preserves_latest_values(context);
	test_start_reinitializes_output(context);
	test_release_preparation_survives_start(context);
	test_config_is_owned_by_core(context);
	test_invalid_config_rejected(context);
	test_frame_output_isolation(context);
	test_simulation_pipeline(context);
	test_cockpit_inputs_drive_outputs(context);
	test_neutral_cockpit_input_completes_step(context);
	test_damage_returns_immediate_result(context);
}
