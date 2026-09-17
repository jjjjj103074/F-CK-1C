#include "TestHarness.h"
#include "Fck1cEfmTestFixture.h"

#include "Core/Fck1cEfm.h"

#include <stdexcept>

namespace
{
constexpr double kTolerance = 1e-9;
constexpr double kSimulationStepS = 0.01;
constexpr double kPreparedInternalFuel = 240.0;
constexpr double kPreparedExternalFuelLeft = 20.0;
constexpr double kPreparedExternalFuelRight = 30.0;
constexpr double kExpectedIndicatedAirspeedMps = 137.484693025538;
constexpr double kExpectedVerticalSpeedMps = 4.0;
constexpr double kExpectedWorldYawRad = 0.3;
constexpr double kExpectedPitchAttitudeRad = 0.1;
constexpr double kExpectedRollAttitudeRad = -0.2;
constexpr double kExpectedRollRateRadS = 0.05;
constexpr double kExpectedPitchRateRadS = 0.07;
constexpr double kExpectedYawRateRadS = 0.06;
constexpr int kLeftExternalFuelStation = 1;
constexpr int kRightExternalFuelStation = 2;
constexpr std::size_t kFirstDamageSegment = 0;
constexpr double kDamagedIntegrity = 0.2;
constexpr double kCommandEnabled = 1.0;
constexpr double kLifecyclePitchInput = 0.5;
constexpr Core::StartMode kLifecycleModes[] = {
	Core::StartMode::HotGround,
	Core::StartMode::HotAir,
	Core::StartMode::ColdGround
};

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
	TEST_EXPECT(context, output.cockpit.status.available);
	TEST_EXPECT(context, output.cockpit.status.revision == 0);
	TEST_EXPECT_NEAR(
		context, output.cockpit.simulation_time_s, 0.0, kTolerance);
	TEST_EXPECT(context, output.engines[0].switch_on == hot);
	TEST_EXPECT(context, output.engines[1].switch_on == hot);
	TEST_EXPECT_NEAR(
		context, output.landing_gear.gear_position_normalized,
		airborne ? 0.0 : 1.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, output.engines[0].throttle_input_normalized,
		airborne ? 0.5 : 0.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, output.engines[1].throttle_output_normalized,
		hot ? 0.5 : 0.0, kTolerance);
}

void expect_engine_baseline(
	Tests::Context& context,
	const Core::EngineOutput& actual)
{
	TEST_EXPECT(context, actual.switch_on);
	TEST_EXPECT_NEAR(context, actual.throttle_input_normalized, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, actual.throttle_output_normalized,
		0.49384615384615382, kTolerance);
	TEST_EXPECT_NEAR(context, actual.power_readout_normalized, 0.5, kTolerance);
	TEST_EXPECT_NEAR(context, actual.thrust_force_n,
		12742.589350616381, kTolerance);
	TEST_EXPECT_NEAR(context, actual.afterburner_ratio_0_1, 0.0, kTolerance);
	TEST_EXPECT(context, !actual.afterburner_lit);
	TEST_EXPECT_NEAR(context, actual.nozzle_aperture_normalized,
		0.39453125, kTolerance);
}

void expect_control_baseline(
	Tests::Context& context,
	const Core::ControlOutput& actual)
{
	TEST_EXPECT_NEAR(context, actual.pitch_input_normalized, 0.2, kTolerance);
	TEST_EXPECT_NEAR(context, actual.roll_input_normalized, -0.3, kTolerance);
	TEST_EXPECT_NEAR(context, actual.yaw_input_normalized, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, actual.symmetric_stabilator_position_rad,
		-0.00072786751501098036, kTolerance);
	TEST_EXPECT_NEAR(context, actual.differential_flaperon_position_rad,
		-0.00054343924852631416, kTolerance);
	TEST_EXPECT_NEAR(context, actual.rudder_position_rad,
		-0.00028827451968722407, kTolerance);
	TEST_EXPECT_NEAR(context, actual.flaps_position_normalized, 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, actual.slats_position_normalized, 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, actual.airbrake_position_normalized, 0.0, kTolerance);
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
		expect_vec3(
			context, actual.wheels[index].acting_force_body_n, forces[index]);
		TEST_EXPECT_NEAR(
			context, actual.wheels[index].compression_m,
			compression[index], kTolerance);
		TEST_EXPECT_NEAR(
			context, actual.wheels[index].force_magnitude_n,
			force_magnitude[index], kTolerance);
		TEST_EXPECT(context, actual.wheels[index].weight_on_wheel);
	}
	TEST_EXPECT(context, actual.any_weight_on_wheels);
	TEST_EXPECT(context, actual.on_ground);
}

void expect_golden_frame(
	Tests::Context& context,
	const Core::FrameOutput& actual)
{
	TEST_EXPECT_NEAR(context, actual.flight.indicated_airspeed_mps,
		kExpectedIndicatedAirspeedMps, kTolerance);
	TEST_EXPECT_NEAR(context, actual.flight.vertical_speed_mps,
		kExpectedVerticalSpeedMps, kTolerance);
	TEST_EXPECT_NEAR(context, actual.flight.world_yaw_rad,
		kExpectedWorldYawRad, kTolerance);
	TEST_EXPECT_NEAR(context, actual.flight.pitch_attitude_rad,
		kExpectedPitchAttitudeRad, kTolerance);
	TEST_EXPECT_NEAR(context, actual.flight.roll_attitude_rad,
		kExpectedRollAttitudeRad, kTolerance);
	TEST_EXPECT_NEAR(context, actual.flight.roll_rate_rad_s,
		kExpectedRollRateRadS, kTolerance);
	TEST_EXPECT_NEAR(context, actual.flight.pitch_rate_rad_s,
		kExpectedPitchRateRadS, kTolerance);
	TEST_EXPECT_NEAR(context, actual.flight.yaw_rate_rad_s,
		kExpectedYawRateRadS, kTolerance);
	expect_vec3(context, actual.force_moment.force_body_n,
		{ 17848.989928072871, 105383.07652268042, 7482.951250460349 });
	expect_vec3(context, actual.force_moment.moment_body_nm,
		{ 32076.378817798446, 610.57122003046698, -30860.167189273627 });
	expect_vec3(
		context, actual.force_moment.center_of_mass_body_m,
		{ 0.2, -0.1, 0.3 });
	expect_engine_baseline(context, actual.engines[0]);
	expect_engine_baseline(context, actual.engines[1]);
	TEST_EXPECT_NEAR(context, actual.fuel.internal_fuel_kg, 500.0, kTolerance);
	TEST_EXPECT_NEAR(context, actual.fuel.external_fuel_kg, 120.0, kTolerance);
	TEST_EXPECT_NEAR(context, actual.fuel.total_fuel_kg, 620.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, actual.fuel.total_fuel_flow_kg_s, 0.0, kTolerance);
	expect_control_baseline(context, actual.controls);
	TEST_EXPECT_NEAR(
		context, actual.landing_gear.brake_left_normalized, 0.4, kTolerance);
	TEST_EXPECT_NEAR(
		context, actual.landing_gear.brake_right_normalized, 0.6, kTolerance);
	expect_suspension_baseline(context, actual.suspension);
}

void test_complete_frame_input_contract(Tests::Context& context)
{
	const Core::FrameInput input = make_frame_input();
	TEST_EXPECT_NEAR(context, input.dt_s, 0.02, kTolerance);
	TEST_EXPECT_NEAR(context, input.atmosphere.pressure_pa, 88000.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, input.mass.moment_of_inertia_body_kg_m2.z, 13.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, input.world_kinematics.position_world_m.z, 1200.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, input.body_kinematics.wind_velocity_body_mps.x, 4.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, input.suspension[2].acting_force_point_body_m.z,
		9.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, input.suspension[2].integrity_factor_0_1, 0.7, kTolerance);
	TEST_EXPECT_NEAR(
		context, input.suspension[2].wheel_speed_x_mps, 14.0, kTolerance);
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
		Core::Fck1cEfm efm(
			make_test_config(), Tests::disabled_debug_telemetry());
		const Core::FrameOutput output = efm.start(mode);
		expect_availability(context, output.availability, {});
		expect_start_output(context, output, mode);
	}
}

void test_frame_output_golden_contract(Tests::Context& context)
{
	Core::Fck1cEfm efm(
		make_test_config(), Tests::disabled_debug_telemetry());
	(void)efm.start(Core::StartMode::HotGround);
	efm.set_internal_fuel(500.0);
	efm.set_external_fuel({ 1, 120.0, { 0.5, -0.2, 0.1 } });
	efm.handle_command({
		Core::CommandId::SetLeftBrake, 0.4 });
	efm.handle_command({
		Core::CommandId::SetRightBrake, 0.6 });
	efm.handle_command({
		Core::CommandId::SetPitchAxis, 0.2 });
	efm.handle_command({
		Core::CommandId::SetRollAxis, -0.3 });
	efm.handle_command({
		Core::CommandId::SetCommonThrottleAxis, 0.2 });
	const Core::FrameInput input = make_frame_input();
	const Core::FrameOutput output = efm.step(input);
	// Rebaselined for the typed 256 Hz flight-control actuation seam.
	expect_golden_frame(context, output);
	expect_availability(context, output.availability, input.availability);
	TEST_EXPECT_NEAR(context, output.simulation_time_s, input.dt_s, kTolerance);
}

void test_unavailable_input_preserves_latest_values(Tests::Context& context)
{
	Core::Fck1cEfm efm(
		make_test_config(), Tests::disabled_debug_telemetry());
	(void)efm.start(Core::StartMode::HotGround);
	const Core::FrameOutput first = efm.step(make_frame_input());
	Core::FrameInput next_input;
	next_input.dt_s = kSimulationStepS;
	const Core::FrameOutput next = efm.step(next_input);
	expect_availability(context, next.availability, {});
	TEST_EXPECT_NEAR(
		context, next.flight.altitude_asl_m, first.flight.altitude_asl_m, kTolerance);
	expect_vec3(context, next.force_moment.center_of_mass_body_m,
		first.force_moment.center_of_mass_body_m);
	for (std::size_t index = 0; index < next.suspension.wheels.size(); ++index)
	{
		expect_vec3(
			context,
			next.suspension.wheels[index].acting_force_body_n,
			first.suspension.wheels[index].acting_force_body_n);
		TEST_EXPECT_NEAR(
			context,
			next.suspension.wheels[index].compression_m,
			first.suspension.wheels[index].compression_m,
			kTolerance);
	}
}

void test_start_reinitializes_output(Tests::Context& context)
{
	auto config = make_test_config();
	config.engine.fuel_consumption_rate_kg_s = 3.0;
	Core::Fck1cEfm efm(config, Tests::disabled_debug_telemetry());
	(void)efm.start(Core::StartMode::HotGround);
	const Core::FrameOutput previous = efm.step(make_frame_input());
	TEST_EXPECT(context, previous.fuel.total_fuel_flow_kg_s > 0.0);
	const Core::FrameOutput output = efm.start(Core::StartMode::ColdGround);
	TEST_EXPECT_NEAR(context, output.simulation_time_s, 0.0, kTolerance);
	TEST_EXPECT(context, output.cockpit.status.revision == 0);
	TEST_EXPECT_NEAR(context, output.flight.altitude_asl_m, 0.0, kTolerance);
	expect_vec3(context, output.force_moment.force_body_n, {});
	expect_vec3(context, output.force_moment.moment_body_nm, {});
	TEST_EXPECT_NEAR(
		context, output.controls.flaps_position_normalized, 0.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, output.suspension.wheels[0].compression_m, 0.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, output.engines[0].thrust_force_n, 0.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, output.fuel.total_fuel_flow_kg_s, 0.0, kTolerance);
}

void test_release_preparation_survives_start(Tests::Context& context)
{
	auto config = make_test_config();
	config.engine.fuel_consumption_rate_kg_s = 3.0;
	Core::Fck1cEfm efm(config, Tests::disabled_debug_telemetry());
	(void)efm.start(Core::StartMode::ColdGround);
	efm.set_internal_fuel(100.0);
	const Core::FrameOutput easy_flight_disabled =
		efm.step(make_frame_input());
	efm.release();
	efm.set_internal_fuel(200.0);
	efm.set_external_fuel({ 1, 30.0, {} });
	efm.set_infinite_fuel(true);
	efm.set_infinite_fuel(true);
	efm.set_easy_flight(true);
	efm.set_invincible(true);
	const Core::FrameOutput start = efm.start(Core::StartMode::ColdGround);
	TEST_EXPECT_NEAR(context, start.fuel.internal_fuel_kg, 200.0, kTolerance);
	TEST_EXPECT_NEAR(context, start.fuel.external_fuel_kg, 30.0, kTolerance);
	TEST_EXPECT_NEAR(context, start.fuel.total_fuel_kg, 230.0, kTolerance);
	const Core::DamageEvent damage = { Core::DamageArea::LeftWing, 0, 0.2 };
	TEST_EXPECT(context, efm.apply_damage(damage).invincible);
	const Core::FrameOutput next = efm.step(make_frame_input());
	TEST_EXPECT_NEAR(context, next.fuel.internal_fuel_kg, 200.0, kTolerance);
	TEST_EXPECT_NEAR(context, next.fuel.external_fuel_kg, 30.0, kTolerance);
	TEST_EXPECT(context, !next.mass_effect.available);
	TEST_EXPECT(
		context,
		std::fabs(next.force_moment.moment_body_nm.x -
			easy_flight_disabled.force_moment.moment_body_nm.x) > kTolerance);
}

bool step_throws_without_active_flight(Core::Fck1cEfm& efm)
{
	try
	{
		(void)efm.step(make_frame_input());
	}
	catch (const std::logic_error&)
	{
		return true;
	}
	return false;
}

void test_step_requires_active_flight(Tests::Context& context)
{
	Core::Fck1cEfm efm(
		make_test_config(), Tests::disabled_debug_telemetry());
	TEST_EXPECT(context, step_throws_without_active_flight(efm));
	(void)efm.start(Core::StartMode::HotGround);
	efm.release();
	TEST_EXPECT(context, step_throws_without_active_flight(efm));
}

void configure_active_preparation(Core::Fck1cEfm& efm)
{
	efm.set_internal_fuel(kPreparedInternalFuel);
	efm.set_external_fuel({
		kLeftExternalFuelStation, kPreparedExternalFuelLeft, {} });
	efm.set_external_fuel({
		kRightExternalFuelStation, kPreparedExternalFuelRight, {} });
	efm.set_infinite_fuel(true);
	efm.set_easy_flight(true);
	efm.set_invincible(true);
}

void expect_prepared_fuel(
	Tests::Context& context,
	const Core::FrameOutput& output)
{
	const double external =
		kPreparedExternalFuelLeft + kPreparedExternalFuelRight;
	TEST_EXPECT_NEAR(
		context, output.fuel.internal_fuel_kg, kPreparedInternalFuel, kTolerance);
	TEST_EXPECT_NEAR(context, output.fuel.external_fuel_kg, external, kTolerance);
	TEST_EXPECT_NEAR(
		context, output.fuel.total_fuel_kg,
		kPreparedInternalFuel + external, kTolerance);
}

void test_active_preparation_updates_next_flight(Tests::Context& context)
{
	Core::Fck1cEfm efm(
		make_test_config(), Tests::disabled_debug_telemetry());
	Core::Fck1cEfm normal_flight(
		make_test_config(), Tests::disabled_debug_telemetry());
	(void)efm.start(Core::StartMode::HotGround);
	configure_active_preparation(efm);
	expect_prepared_fuel(context, efm.step(make_frame_input()));
	const Core::FrameOutput restarted = efm.start(Core::StartMode::HotAir);
	expect_prepared_fuel(context, restarted);
	TEST_EXPECT(context, efm.apply_damage(
		{
			Core::DamageArea::LeftWing,
			kFirstDamageSegment,
			kDamagedIntegrity
		}).invincible);
	const Core::FrameOutput easy_flight = efm.step(make_frame_input());
	expect_prepared_fuel(context, easy_flight);
	(void)normal_flight.start(Core::StartMode::HotAir);
	const Core::FrameOutput normal = normal_flight.step(make_frame_input());
	TEST_EXPECT(context, std::fabs(
		easy_flight.force_moment.moment_body_nm.x -
		normal.force_moment.moment_body_nm.x) > kTolerance);
}

void test_release_synchronizes_consumed_fuel(Tests::Context& context)
{
	auto config = make_test_config();
	config.engine.fuel_consumption_rate_kg_s = 3.0;
	Core::Fck1cEfm efm(config, Tests::disabled_debug_telemetry());
	(void)efm.start(Core::StartMode::HotAir);
	efm.set_internal_fuel(kPreparedInternalFuel);
	efm.set_external_fuel({
		kLeftExternalFuelStation, kPreparedExternalFuelLeft, {} });
	const Core::FrameOutput consumed = efm.step(make_frame_input());
	efm.release();
	const Core::FrameOutput restarted = efm.start(Core::StartMode::HotAir);
	TEST_EXPECT_NEAR(context, restarted.fuel.internal_fuel_kg,
		consumed.fuel.internal_fuel_kg, kTolerance);
	TEST_EXPECT_NEAR(context, restarted.fuel.external_fuel_kg,
		consumed.fuel.external_fuel_kg, kTolerance);
}

void test_active_restart_synchronizes_consumed_fuel(Tests::Context& context)
{
	auto config = make_test_config();
	config.engine.fuel_consumption_rate_kg_s = 3.0;
	Core::Fck1cEfm efm(config, Tests::disabled_debug_telemetry());
	(void)efm.start(Core::StartMode::HotAir);
	efm.set_internal_fuel(kPreparedInternalFuel);
	efm.set_external_fuel({
		kLeftExternalFuelStation, kPreparedExternalFuelLeft, {} });
	const Core::FrameOutput consumed = efm.step(make_frame_input());
	const Core::FrameOutput restarted = efm.start(Core::StartMode::HotAir);
	TEST_EXPECT_NEAR(context, restarted.fuel.internal_fuel_kg,
		consumed.fuel.internal_fuel_kg, kTolerance);
	TEST_EXPECT_NEAR(context, restarted.fuel.external_fuel_kg,
		consumed.fuel.external_fuel_kg, kTolerance);
}

void test_released_commands_and_damage_do_not_persist(Tests::Context& context)
{
	Core::Fck1cEfm subject(
		make_test_config(), Tests::disabled_debug_telemetry());
	Core::Fck1cEfm baseline(
		make_test_config(), Tests::disabled_debug_telemetry());
	(void)subject.start(Core::StartMode::HotAir);
	subject.release();
	subject.repair({});
	subject.handle_command({
		Core::CommandId::SetGear,
		kCommandEnabled
	});
	TEST_EXPECT(context, !subject.apply_damage(
		{
			Core::DamageArea::LeftWing,
			kFirstDamageSegment,
			kDamagedIntegrity
		}).invincible);
	(void)subject.start(Core::StartMode::HotAir);
	(void)baseline.start(Core::StartMode::HotAir);
	const Core::FrameOutput actual = subject.step(make_frame_input());
	const Core::FrameOutput expected = baseline.step(make_frame_input());
	TEST_EXPECT_NEAR(context, actual.landing_gear.gear_position_normalized,
		expected.landing_gear.gear_position_normalized, kTolerance);
	expect_vec3(
		context, actual.force_moment.force_body_n,
		expected.force_moment.force_body_n);
	expect_vec3(
		context, actual.force_moment.moment_body_nm,
		expected.force_moment.moment_body_nm);
}

void test_repeated_start_release_cycles(Tests::Context& context)
{
	Core::Fck1cEfm efm(
		make_test_config(), Tests::disabled_debug_telemetry());
	for (const Core::StartMode mode : kLifecycleModes)
	{
		expect_start_output(context, efm.start(mode), mode);
		efm.handle_command({
			Core::CommandId::SetPitchAxis,
			kLifecyclePitchInput
		});
		(void)efm.step(make_frame_input());
		efm.release();
	}
}

void test_config_is_owned_by_core(Tests::Context& context)
{
	auto source = make_test_config();
	Core::Fck1cEfm efm(source, Tests::disabled_debug_telemetry());
	source.propulsion.max_thrust_table_n[0] = 0.0;
	(void)efm.start(Core::StartMode::HotGround);
	efm.set_internal_fuel(100.0);
	Core::FrameInput input;
	input.dt_s = kSimulationStepS;
	const Core::FrameOutput output = efm.step(input);
	TEST_EXPECT(context, output.engines[0].thrust_force_n > 0.0);
	TEST_EXPECT(context, output.engines[1].thrust_force_n > 0.0);
}

void test_invalid_config_rejected(Tests::Context& context)
{
	bool rejected = false;
	try
	{
		Core::Fck1cEfm efm(
			Tests::Fck1c::TestAircraftConfig{},
			Tests::disabled_debug_telemetry());
		(void)efm.start(Core::StartMode::HotGround);
	}
	catch (const std::invalid_argument&)
	{
		rejected = true;
	}
	TEST_EXPECT(context, rejected);
}

void test_frame_output_isolation(Tests::Context& context)
{
	Core::Fck1cEfm efm(
		make_test_config(), Tests::disabled_debug_telemetry());
	(void)efm.start(Core::StartMode::HotGround);
	efm.set_internal_fuel(1200.0);
	Core::FrameInput input;
	input.dt_s = kSimulationStepS;
	Core::FrameOutput copy = efm.step(input);
	copy.fuel.internal_fuel_kg = 2.0;
	copy.controls.pitch_input_normalized = 3.0;
	const Core::FrameOutput current = efm.step(input);
	TEST_EXPECT(context, current.fuel.internal_fuel_kg > 1000.0);
	TEST_EXPECT(context, current.controls.pitch_input_normalized != 3.0);
}

void test_simulation_pipeline(Tests::Context& context)
{
	Core::Fck1cEfm efm(
		make_test_config(), Tests::disabled_debug_telemetry());
	(void)efm.start(Core::StartMode::ColdGround);
	efm.set_internal_fuel(100.0);
	Core::FrameInput input;
	input.dt_s = kSimulationStepS;
	const Core::FrameOutput first = efm.step(input);
	TEST_EXPECT_NEAR(context, first.simulation_time_s, kSimulationStepS, kTolerance);
	TEST_EXPECT(context, first.cockpit.status.available);
	TEST_EXPECT(context, first.cockpit.status.revision == 1);
	TEST_EXPECT_NEAR(
		context,
		first.cockpit.simulation_time_s,
		first.simulation_time_s,
		kTolerance);
	TEST_EXPECT(
		context,
		first.cockpit.automatic_flight_control.status.available);
	TEST_EXPECT(
		context,
		first.cockpit.automatic_flight_control.status.revision == 0);
	const Core::FrameOutput second = efm.step(input);
	TEST_EXPECT_NEAR(
		context, second.simulation_time_s, kSimulationStepS * 2.0, kTolerance);
	TEST_EXPECT(context, second.cockpit.status.revision == 2);
}

}

void run_fck1c_efm_tests(Tests::Context& context)
{
	test_complete_frame_input_contract(context);
	test_all_start_mode_outputs(context);
	test_frame_output_golden_contract(context);
	test_unavailable_input_preserves_latest_values(context);
	test_start_reinitializes_output(context);
	test_release_preparation_survives_start(context);
	test_step_requires_active_flight(context);
	test_active_preparation_updates_next_flight(context);
	test_release_synchronizes_consumed_fuel(context);
	test_active_restart_synchronizes_consumed_fuel(context);
	test_released_commands_and_damage_do_not_persist(context);
	test_repeated_start_release_cycles(context);
	test_config_is_owned_by_core(context);
	test_invalid_config_rejected(context);
	test_frame_output_isolation(context);
	test_simulation_pipeline(context);
}
