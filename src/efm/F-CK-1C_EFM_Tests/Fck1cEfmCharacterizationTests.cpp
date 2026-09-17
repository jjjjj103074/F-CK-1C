#include "TestHarness.h"
#include "Fck1cEfmTestFixture.h"

#include <array>
#include <cstdint>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>

namespace
{
constexpr double kTolerance = 1e-9;
constexpr int kDoubleRoundTripPrecision = 17;
constexpr std::size_t kMaxGearTransitionFrames = 1000;
constexpr double kGearMidpoint = 0.5;
constexpr double kExpectedFlapIncrementPerFrame = 0.002;
constexpr double kExpectedSlatIncrementPerFrame = 0.003;
constexpr double kEngineShutdownAltitudeAsl = 21000.0;
constexpr double kOperatingFuelMass = 100.0;
constexpr double kSystemPeriodS = 1.0 / 64.0;
constexpr std::size_t kCharacterizationFrameCount = 4;
constexpr std::uint64_t kFnvOffsetBasis = 14695981039346656037ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;
constexpr std::array<std::uint64_t, 4> kSchedulerTrajectoryHashes = {
	12341418355830989210ULL,
	16376143566750479751ULL,
	7243827386340887530ULL,
	11166130031442543139ULL
};

void write_availability(
	std::ostringstream& output,
	const Core::FrameDataAvailability& value)
{
	output << "availability.atmosphere=" << value.atmosphere << '\n';
	output << "availability.surface=" << value.surface << '\n';
	output << "availability.mass=" << value.mass << '\n';
	output << "availability.world_kinematics=" << value.world_kinematics << '\n';
	output << "availability.body_kinematics=" << value.body_kinematics << '\n';
	for (std::size_t index = 0; index < value.suspension.size(); ++index)
	{
		output << "availability.suspension[" << index << "]="
			<< value.suspension[index] << '\n';
	}
}

void write_flight(std::ostringstream& output, const Core::FlightOutput& value)
{
	output << "flight.altitude_asl_m=" << value.altitude_asl_m << '\n';
	output << "flight.altitude_agl_m=" << value.altitude_agl_m << '\n';
	output << "flight.position_world_z_m=" << value.position_world_z_m << '\n';
	output << "flight.mach=" << value.mach << '\n';
	output << "flight.normal_acceleration_g="
		<< value.normal_acceleration_g << '\n';
	output << "flight.angle_of_attack_deg=" << value.angle_of_attack_deg << '\n';
	output << "flight.angle_of_slide_deg=" << value.angle_of_slide_deg << '\n';
	output << "flight.atmosphere_temperature_k="
		<< value.atmosphere_temperature_k << '\n';
	output << "flight.indicated_airspeed_mps="
		<< value.indicated_airspeed_mps << '\n';
	output << "flight.vertical_speed_mps=" << value.vertical_speed_mps << '\n';
	output << "flight.world_yaw_rad=" << value.world_yaw_rad << '\n';
	output << "flight.pitch_attitude_rad=" << value.pitch_attitude_rad << '\n';
	output << "flight.roll_attitude_rad=" << value.roll_attitude_rad << '\n';
	output << "flight.roll_rate_rad_s=" << value.roll_rate_rad_s << '\n';
	output << "flight.pitch_rate_rad_s=" << value.pitch_rate_rad_s << '\n';
	output << "flight.yaw_rate_rad_s=" << value.yaw_rate_rad_s << '\n';
}

void write_vec3(
	std::ostringstream& output,
	const char* name,
	const Common::Vec3& value)
{
	output << name << ".x=" << value.x << '\n';
	output << name << ".y=" << value.y << '\n';
	output << name << ".z=" << value.z << '\n';
}

void write_engine(
	std::ostringstream& output,
	std::size_t index,
	const Core::EngineOutput& value)
{
	const std::string prefix = "engines[" + std::to_string(index) + "].";
	output << prefix << "switch_on=" << value.switch_on << '\n';
	output << prefix << "throttle_input_normalized="
		<< value.throttle_input_normalized << '\n';
	output << prefix << "throttle_output_normalized="
		<< value.throttle_output_normalized << '\n';
	output << prefix << "power_readout_normalized="
		<< value.power_readout_normalized << '\n';
	output << prefix << "thrust_force_n=" << value.thrust_force_n << '\n';
	output << prefix << "afterburner_ratio_0_1="
		<< value.afterburner_ratio_0_1 << '\n';
	output << prefix << "afterburner_lit=" << value.afterburner_lit << '\n';
	output << prefix << "nozzle_aperture_normalized="
		<< value.nozzle_aperture_normalized << '\n';
}

void write_controls(
	std::ostringstream& output,
	const Core::ControlOutput& value)
{
	output << "controls.pitch_input_normalized="
		<< value.pitch_input_normalized << '\n';
	output << "controls.roll_input_normalized="
		<< value.roll_input_normalized << '\n';
	output << "controls.yaw_input_normalized="
		<< value.yaw_input_normalized << '\n';
	output << "controls.symmetric_stabilator_position_rad="
		<< value.symmetric_stabilator_position_rad << '\n';
	output << "controls.differential_flaperon_position_rad="
		<< value.differential_flaperon_position_rad << '\n';
	output << "controls.rudder_position_rad="
		<< value.rudder_position_rad << '\n';
	output << "controls.flaps_position_normalized="
		<< value.flaps_position_normalized << '\n';
	output << "controls.slats_position_normalized="
		<< value.slats_position_normalized << '\n';
	output << "controls.airbrake_position_normalized="
		<< value.airbrake_position_normalized << '\n';
}

void write_landing_gear(
	std::ostringstream& output,
	const Core::LandingGearOutput& value)
{
	output << "landing_gear.gear_position_normalized="
		<< value.gear_position_normalized << '\n';
	output << "landing_gear.nose_wheel_steering_normalized="
		<< value.nose_wheel_steering_normalized << '\n';
	output << "landing_gear.brake_left_normalized="
		<< value.brake_left_normalized << '\n';
	output << "landing_gear.brake_right_normalized="
		<< value.brake_right_normalized << '\n';
	for (std::size_t index = 0;
		index < value.wheel_spin_phase_0_1.size();
		++index)
	{
		output << "landing_gear.wheel_spin_phase_0_1[" << index << "]="
			<< value.wheel_spin_phase_0_1[index] << '\n';
	}
}

void write_suspension_wheel(
	std::ostringstream& output,
	std::size_t index,
	const Core::SuspensionWheelOutput& value)
{
	const std::string prefix = "suspension.wheels[" +
		std::to_string(index) + "].";
	write_vec3(output, (prefix + "acting_force_body_n").c_str(),
		value.acting_force_body_n);
	output << prefix << "compression_m=" << value.compression_m << '\n';
	output << prefix << "force_magnitude_n=" << value.force_magnitude_n << '\n';
	output << prefix << "weight_on_wheel=" << value.weight_on_wheel << '\n';
}

void write_suspension(
	std::ostringstream& output,
	const Core::SuspensionOutput& value)
{
	for (std::size_t index = 0; index < value.wheels.size(); ++index)
	{
		write_suspension_wheel(output, index, value.wheels[index]);
	}
	output << "suspension.any_weight_on_wheels="
		<< value.any_weight_on_wheels << '\n';
	output << "suspension.on_ground=" << value.on_ground << '\n';
}

void write_fuel(std::ostringstream& output, const Core::FuelOutput& value)
{
	output << "fuel.internal_fuel_kg=" << value.internal_fuel_kg << '\n';
	output << "fuel.external_fuel_kg=" << value.external_fuel_kg << '\n';
	output << "fuel.total_fuel_kg=" << value.total_fuel_kg << '\n';
	output << "fuel.total_fuel_flow_kg_s="
		<< value.total_fuel_flow_kg_s << '\n';
}

void write_mass_effect(
	std::ostringstream& output,
	const Core::MassDeltaResult& value)
{
	output << "mass_effect.available=" << value.available << '\n';
	output << "mass_effect.delta.mass_kg=" << value.delta.mass_kg << '\n';
	write_vec3(
		output,
		"mass_effect.delta.position_body_m",
		value.delta.position_body_m);
	write_vec3(
		output,
		"mass_effect.delta.moment_of_inertia_delta_kg_m2",
		value.delta.moment_of_inertia_delta_kg_m2);
}

std::string frame_snapshot(const Core::FrameOutput& frame)
{
	std::ostringstream output;
	output << std::setprecision(kDoubleRoundTripPrecision) << std::boolalpha;
	output << "simulation_time_s=" << frame.simulation_time_s << '\n';
	write_availability(output, frame.availability);
	write_flight(output, frame.flight);
	write_vec3(output, "force_moment.force_body_n", frame.force_moment.force_body_n);
	write_vec3(
		output, "force_moment.moment_body_nm", frame.force_moment.moment_body_nm);
	write_vec3(
		output,
		"force_moment.center_of_mass_body_m",
		frame.force_moment.center_of_mass_body_m);
	for (std::size_t index = 0; index < frame.engines.size(); ++index)
	{
		write_engine(output, index, frame.engines[index]);
	}
	write_controls(output, frame.controls);
	write_landing_gear(output, frame.landing_gear);
	write_suspension(output, frame.suspension);
	write_fuel(output, frame.fuel);
	write_mass_effect(output, frame.mass_effect);
	output << "shake_amplitude_normalized="
		<< frame.shake_amplitude_normalized << '\n';
	return output.str();
}

void send_trajectory_commands(Core::Fck1cEfm& efm)
{
	efm.handle_command({
		Core::CommandId::SetPitchAxis, 0.25 });
	efm.handle_command({
		Core::CommandId::SetRollAxis, -0.2 });
	efm.handle_command({
		Core::CommandId::SetYawAxis, 0.15 });
	efm.handle_command({
		Core::CommandId::SetCommonThrottleAxis, 0.8 });
	efm.handle_command({
		Core::CommandId::SetGear, 0.0 });
	efm.handle_command({
		Core::CommandId::SetFlapsAuto, 1.0 });
	efm.handle_command({
		Core::CommandId::SetAirbrake, 1.0 });
}

std::array<
	Core::FrameOutput,
	kCharacterizationFrameCount> run_trajectory()
{
	Tests::Fck1c::TestAircraftConfig config = Tests::Fck1c::make_test_config();
	config.engine.fuel_consumption_rate_kg_s = 3.0;
	Core::Fck1cEfm efm(config, Tests::disabled_debug_telemetry());
	(void)efm.start(Core::StartMode::HotGround);
	efm.set_internal_fuel(500.0);
	efm.set_external_fuel({ 1, 120.0, { 0.5, -0.2, 0.1 } });
	send_trajectory_commands(efm);

	Core::FrameInput input = Tests::Fck1c::make_frame_input();
	std::array<
		Core::FrameOutput,
		kCharacterizationFrameCount> frames;
	frames[0] = efm.step(input);
	(void)efm.apply_damage({ Core::DamageArea::LeftEngine, 0, 0.6 });
	frames[1] = efm.step(input);
	efm.repair({});
	Core::FrameInput sticky_input;
	sticky_input.dt_s = input.dt_s;
	frames[2] = efm.step(sticky_input);
	frames[3] = efm.step(input);
	return frames;
}

std::uint64_t snapshot_hash(const std::string& snapshot)
{
	std::uint64_t result = kFnvOffsetBasis;
	for (const unsigned char value : snapshot)
	{
		result ^= value;
		result *= kFnvPrime;
	}
	return result;
}

void expect_snapshot_hash(
	Tests::Context& context,
	const std::string& actual,
	std::uint64_t expected)
{
	const std::uint64_t hash = snapshot_hash(actual);
	if (hash != expected)
	{
		std::printf(
			"Actual frame snapshot:\n%sExpected hash: %llu; actual hash: %llu\n",
			actual.c_str(),
			static_cast<unsigned long long>(expected),
			static_cast<unsigned long long>(hash));
	}
	TEST_EXPECT(context, hash == expected);
}

void test_scheduler_multiframe_golden_trajectory(
	Tests::Context& context)
{
	const auto actual = run_trajectory();
	for (std::size_t index = 0; index < actual.size(); ++index)
	{
		expect_snapshot_hash(
			context,
			frame_snapshot(actual[index]),
			kSchedulerTrajectoryHashes[index]);
	}
}

void test_repeated_run_is_deterministic(Tests::Context& context)
{
	const auto first = run_trajectory();
	const auto second = run_trajectory();
	for (std::size_t index = 0; index < first.size(); ++index)
	{
		TEST_EXPECT(
			context, frame_snapshot(first[index]) == frame_snapshot(second[index]));
	}
}

struct GearCrossing
{
	Core::FrameOutput before;
	Core::FrameOutput at;
};

std::optional<GearCrossing> find_gear_midpoint_crossing(
	Core::Fck1cEfm& efm,
	const Core::FrameInput& input,
	Core::FrameOutput previous)
{
	for (std::size_t frame = 0; frame < kMaxGearTransitionFrames; ++frame)
	{
		const Core::FrameOutput current = efm.step(input);
		if (previous.landing_gear.gear_position_normalized <= kGearMidpoint &&
			current.landing_gear.gear_position_normalized > kGearMidpoint)
		{
			return GearCrossing{ previous, current };
		}
		previous = current;
	}
	return std::nullopt;
}

void test_secondary_controls_read_previous_committed_gear(
	Tests::Context& context)
{
	Core::Fck1cEfm efm(
		Tests::Fck1c::make_test_config(),
		Tests::disabled_debug_telemetry());
	const Core::FrameOutput start = efm.start(Core::StartMode::HotAir);
	efm.handle_command({
		Core::CommandId::SetGear, 1.0 });
	efm.handle_command({
		Core::CommandId::SetFlapsAuto, 1.0 });
	Core::FrameInput input = Tests::Fck1c::make_frame_input();
	const std::optional<GearCrossing> crossing =
		find_gear_midpoint_crossing(efm, input, start);
	TEST_EXPECT(context, crossing.has_value());
	if (!crossing)
	{
		return;
	}
	TEST_EXPECT_NEAR(
		context,
		crossing->at.controls.flaps_position_normalized -
			crossing->before.controls.flaps_position_normalized,
		0.0,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		crossing->at.controls.slats_position_normalized -
			crossing->before.controls.slats_position_normalized,
		0.0,
		kTolerance);
	const Core::FrameOutput next = efm.step(input);
	TEST_EXPECT_NEAR(
		context,
		next.controls.flaps_position_normalized -
			crossing->at.controls.flaps_position_normalized,
		kExpectedFlapIncrementPerFrame,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		next.controls.slats_position_normalized -
			crossing->at.controls.slats_position_normalized,
		kExpectedSlatIncrementPerFrame,
		kTolerance);
}

double expected_fuel_flow(
	const Tests::Fck1c::TestAircraftConfig& config,
	const Core::FrameOutput& frame)
{
	const double afterburner_average = 0.5 *
		(frame.engines[0].afterburner_ratio_0_1 +
			frame.engines[1].afterburner_ratio_0_1);
	const double afterburner_factor = 1.0 + afterburner_average *
		(config.engine.afterburner.fuel_factor - 1.0);
	return config.engine.fuel_consumption_rate_kg_s *
		((frame.engines[0].throttle_output_normalized +
			frame.engines[1].throttle_output_normalized + 1.0) / 3.0) *
		afterburner_factor;
}

void test_fuel_reads_previous_committed_engine_demand(
	Tests::Context& context)
{
	Tests::Fck1c::TestAircraftConfig config = Tests::Fck1c::make_test_config();
	config.engine.fuel_consumption_rate_kg_s = 3.0;
	Core::Fck1cEfm efm(config, Tests::disabled_debug_telemetry());
	const Core::FrameOutput start = efm.start(Core::StartMode::HotGround);
	efm.set_internal_fuel(100.0);
	efm.handle_command({
		Core::CommandId::SetCommonThrottleAxis, 1.0 });
	Core::FrameInput input;
	input.dt_s = kSystemPeriodS;
	const Core::FrameOutput first = efm.step(input);
	TEST_EXPECT(context,
		first.engines[0].throttle_output_normalized !=
			start.engines[0].throttle_output_normalized);
	TEST_EXPECT_NEAR(
		context,
		first.fuel.total_fuel_flow_kg_s,
		expected_fuel_flow(config, start),
		kTolerance);
	const Core::FrameOutput second = efm.step(input);
	TEST_EXPECT_NEAR(
		context,
		second.fuel.total_fuel_flow_kg_s,
		expected_fuel_flow(config, first),
		kTolerance);
}

Core::FrameOutput run_engine_shutdown_frame(
	double internal_fuel,
	double altitude_asl)
{
	Core::Fck1cEfm efm(
		Tests::Fck1c::make_test_config(),
		Tests::disabled_debug_telemetry());
	(void)efm.start(Core::StartMode::HotGround);
	efm.set_internal_fuel(internal_fuel);
	efm.handle_command({
		Core::CommandId::SetCommonThrottleAxis,
		1.0
	});
	Core::FrameInput input = Tests::Fck1c::make_frame_input();
	input.dt_s = 0.1;
	input.atmosphere.altitude_asl_m = altitude_asl;
	return efm.step(input);
}

void expect_shutdown_thrust_is_inhibited(
	Tests::Context& context,
	const Core::FrameOutput& frame)
{
	TEST_EXPECT(context, frame.engines[0].throttle_output_normalized > 0.0);
	TEST_EXPECT_NEAR(
		context, frame.engines[0].thrust_force_n, 0.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, frame.engines[1].thrust_force_n, 0.0, kTolerance);
}

void test_empty_fuel_inhibits_thrust_after_engine_update(
	Tests::Context& context)
{
	expect_shutdown_thrust_is_inhibited(
		context,
		run_engine_shutdown_frame(0.0, 0.0));
}

void test_excess_altitude_inhibits_thrust_after_engine_update(
	Tests::Context& context)
{
	expect_shutdown_thrust_is_inhibited(
		context,
		run_engine_shutdown_frame(
			kOperatingFuelMass,
			kEngineShutdownAltitudeAsl));
}

Core::FrameInput make_ground_input(bool feedback_available)
{
	Core::FrameInput input;
	input.dt_s = 0.01;
	input.availability.atmosphere = true;
	input.availability.surface = true;
	input.availability.mass = true;
	input.availability.world_kinematics = true;
	input.availability.body_kinematics = true;
	input.availability.suspension = {
		feedback_available, feedback_available, feedback_available
	};
	input.atmosphere = { 2.25, 288.0, 340.0, 1.225, 101325.0, {} };
	input.surface = { 0.0, 0.0, 0, { 0.0, 1.0, 0.0 } };
	input.mass = { 10000.0, {}, {} };
	input.body_kinematics.acceleration_body_mps2 = { 0.0, 9.81, 0.0 };
	return input;
}

Core::FrameOutput run_ground_frame(
	Tests::Fck1c::TestAircraftConfig config,
	bool feedback_available)
{
	Core::Fck1cEfm efm(config, Tests::disabled_debug_telemetry());
	(void)efm.start(Core::StartMode::HotGround);
	efm.set_internal_fuel(100.0);
	return efm.step(make_ground_input(feedback_available));
}

struct PairedHotGroundEfms
{
	PairedHotGroundEfms()
		: subject(
			Tests::Fck1c::make_test_config(),
			Tests::disabled_debug_telemetry()),
		control(
			Tests::Fck1c::make_test_config(),
			Tests::disabled_debug_telemetry())
	{
		(void)subject.start(Core::StartMode::HotGround);
		(void)control.start(Core::StartMode::HotGround);
		subject.set_internal_fuel(100.0);
		control.set_internal_fuel(100.0);
	}

	Core::Fck1cEfm subject;
	Core::Fck1cEfm control;
};

void test_feedback_suppresses_fallback_force(Tests::Context& context)
{
	Tests::Fck1c::TestAircraftConfig fallback = Tests::Fck1c::make_test_config();
	fallback.ground_interaction.enable_fallback_ground_forces = true;
	Tests::Fck1c::TestAircraftConfig disabled = fallback;
	disabled.ground_interaction.enable_fallback_ground_forces = false;
	const Core::FrameOutput with_feedback = run_ground_frame(fallback, true);
	const Core::FrameOutput without_feedback = run_ground_frame(fallback, false);
	const Core::FrameOutput without_fallback = run_ground_frame(disabled, true);
	TEST_EXPECT_NEAR(
		context,
		with_feedback.force_moment.force_body_n.y,
		without_fallback.force_moment.force_body_n.y,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		with_feedback.force_moment.moment_body_nm.z,
		without_fallback.force_moment.moment_body_nm.z,
		kTolerance);
	TEST_EXPECT(context,
		without_feedback.force_moment.force_body_n.y >
			with_feedback.force_moment.force_body_n.y);
}

void test_repair_clears_damage_but_preserves_engine_history(
	Tests::Context& context)
{
	PairedHotGroundEfms pair;
	Core::FrameInput input;
	input.dt_s = 0.02;
	(void)pair.subject.step(input);
	(void)pair.control.step(input);
	(void)pair.subject.apply_damage({ Core::DamageArea::LeftEngine, 0, 0.25 });
	const Core::FrameOutput damaged_frame = pair.subject.step(input);
	const Core::FrameOutput control_frame = pair.control.step(input);
	TEST_EXPECT(context,
		damaged_frame.engines[0].thrust_force_n <
			control_frame.engines[0].thrust_force_n);
	pair.subject.repair({});
	const Core::FrameOutput repaired = pair.subject.step(input);
	const Core::FrameOutput expected = pair.control.step(input);
	TEST_EXPECT(context,
		repaired.engines[0].thrust_force_n >
			damaged_frame.engines[0].thrust_force_n);
	TEST_EXPECT(context,
		repaired.engines[0].thrust_force_n !=
			expected.engines[0].thrust_force_n);
}

void test_invincible_damage_is_discarded(Tests::Context& context)
{
	PairedHotGroundEfms pair;
	pair.subject.set_invincible(true);
	TEST_EXPECT(context, pair.subject.apply_damage(
		{ Core::DamageArea::LeftEngine, 0, 0.2 }).invincible);
	pair.subject.set_invincible(false);
	(void)pair.subject.apply_damage({ Core::DamageArea::RightWing, 0, 1.0 });
	Core::FrameInput input;
	input.dt_s = 0.02;
	const Core::FrameOutput ignored = pair.subject.step(input);
	const Core::FrameOutput undamaged = pair.control.step(input);
	TEST_EXPECT_NEAR(
		context,
		ignored.engines[0].thrust_force_n,
		undamaged.engines[0].thrust_force_n,
		kTolerance);
}

void test_each_frame_exposes_its_mass_effect(Tests::Context& context)
{
	Tests::Fck1c::TestAircraftConfig config = Tests::Fck1c::make_test_config();
	config.engine.fuel_consumption_rate_kg_s = 3.0;
	Core::Fck1cEfm efm(config, Tests::disabled_debug_telemetry());
	(void)efm.start(Core::StartMode::HotGround);
	efm.set_internal_fuel(100.0);
	Core::FrameInput input;
	input.dt_s = kSystemPeriodS;
	const Core::FrameOutput first = efm.step(input);
	const Core::FrameOutput second = efm.step(input);
	TEST_EXPECT(context, first.mass_effect.available);
	TEST_EXPECT(context, second.mass_effect.available);
	TEST_EXPECT_NEAR(
		context,
		first.mass_effect.delta.mass_kg,
		first.fuel.total_fuel_flow_kg_s * input.dt_s,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		second.mass_effect.delta.mass_kg,
		second.fuel.total_fuel_flow_kg_s * input.dt_s,
		kTolerance);
}

void test_infinite_fuel_suppresses_mass_effect(Tests::Context& context)
{
	Tests::Fck1c::TestAircraftConfig config = Tests::Fck1c::make_test_config();
	config.engine.fuel_consumption_rate_kg_s = 3.0;
	Core::Fck1cEfm subject(config, Tests::disabled_debug_telemetry());
	Core::Fck1cEfm control(config, Tests::disabled_debug_telemetry());
	(void)subject.start(Core::StartMode::HotGround);
	(void)control.start(Core::StartMode::HotGround);
	subject.set_internal_fuel(100.0);
	control.set_internal_fuel(100.0);
	Core::FrameInput input;
	input.dt_s = 0.1;
	(void)subject.step(input);
	(void)control.step(input);
	const double fuel_before_kg = subject.internal_fuel_kg();
	subject.set_infinite_fuel(true);
	subject.set_infinite_fuel(true);
	const Core::FrameOutput unlimited = subject.step(input);
	const Core::FrameOutput limited = control.step(input);
	TEST_EXPECT(context, !unlimited.mass_effect.available);
	TEST_EXPECT(context, limited.mass_effect.available);
	TEST_EXPECT_NEAR(
		context,
		unlimited.fuel.total_fuel_flow_kg_s,
		limited.fuel.total_fuel_flow_kg_s,
		kTolerance);
	TEST_EXPECT_NEAR(
		context, subject.internal_fuel_kg(), fuel_before_kg, kTolerance);
	const Core::FrameOutput still_unlimited = subject.step(input);
	const Core::FrameOutput still_limited = control.step(input);
	TEST_EXPECT(context, !still_unlimited.mass_effect.available);
	TEST_EXPECT_NEAR(
		context,
		still_unlimited.fuel.total_fuel_flow_kg_s,
		still_limited.fuel.total_fuel_flow_kg_s,
		kTolerance);
	TEST_EXPECT_NEAR(
		context, subject.internal_fuel_kg(), fuel_before_kg, kTolerance);
	subject.set_infinite_fuel(false);
	subject.set_infinite_fuel(false);
	const Core::FrameOutput resumed = subject.step(input);
	const Core::FrameOutput expected = control.step(input);
	TEST_EXPECT(context, resumed.mass_effect.available);
	TEST_EXPECT_NEAR(
		context,
		resumed.fuel.total_fuel_flow_kg_s,
		expected.fuel.total_fuel_flow_kg_s,
		kTolerance);
	TEST_EXPECT(context, subject.internal_fuel_kg() < fuel_before_kg);
}
}

void run_fck1c_efm_characterization_tests(Tests::Context& context)
{
	test_scheduler_multiframe_golden_trajectory(context);
	test_repeated_run_is_deterministic(context);
	test_secondary_controls_read_previous_committed_gear(context);
	test_fuel_reads_previous_committed_engine_demand(context);
	test_empty_fuel_inhibits_thrust_after_engine_update(context);
	test_excess_altitude_inhibits_thrust_after_engine_update(context);
	test_feedback_suppresses_fallback_force(context);
	test_repair_clears_damage_but_preserves_engine_history(context);
	test_invincible_damage_is_discarded(context);
	test_each_frame_exposes_its_mass_effect(context);
	test_infinite_fuel_suppresses_mass_effect(context);
}
