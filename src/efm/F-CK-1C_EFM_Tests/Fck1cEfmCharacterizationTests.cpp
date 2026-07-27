#include "TestHarness.h"
#include "Fck1cEfmGoldenSnapshots.h"
#include "Fck1cEfmTestFixture.h"

#include <array>
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
	output << "flight.g_load=" << value.g_load << '\n';
	output << "flight.angle_of_attack_deg=" << value.angle_of_attack_deg << '\n';
	output << "flight.angle_of_slide_deg=" << value.angle_of_slide_deg << '\n';
	output << "flight.atmosphere_temperature_k="
		<< value.atmosphere_temperature_k << '\n';
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
	output << prefix << "throttle_input=" << value.throttle_input << '\n';
	output << prefix << "throttle_output=" << value.throttle_output << '\n';
	output << prefix << "power_readout=" << value.power_readout << '\n';
	output << prefix << "thrust_force=" << value.thrust_force << '\n';
	output << prefix << "afterburner_ratio=" << value.afterburner_ratio << '\n';
	output << prefix << "afterburner_lit=" << value.afterburner_lit << '\n';
	output << prefix << "nozzle_aperture=" << value.nozzle_aperture << '\n';
}

void write_controls(
	std::ostringstream& output,
	const Core::ControlOutput& value)
{
	output << "controls.pitch_input=" << value.pitch_input << '\n';
	output << "controls.roll_input=" << value.roll_input << '\n';
	output << "controls.yaw_input=" << value.yaw_input << '\n';
	output << "controls.elevator_command=" << value.elevator_command << '\n';
	output << "controls.aileron_command=" << value.aileron_command << '\n';
	output << "controls.rudder_command=" << value.rudder_command << '\n';
	output << "controls.flaps_position=" << value.flaps_position << '\n';
	output << "controls.slats_position=" << value.slats_position << '\n';
	output << "controls.airbrake_position=" << value.airbrake_position << '\n';
}

void write_landing_gear(
	std::ostringstream& output,
	const Core::LandingGearOutput& value)
{
	output << "landing_gear.gear_position=" << value.gear_position << '\n';
	output << "landing_gear.nose_wheel_steering="
		<< value.nose_wheel_steering << '\n';
	output << "landing_gear.brake_left=" << value.brake_left << '\n';
	output << "landing_gear.brake_right=" << value.brake_right << '\n';
	for (std::size_t index = 0; index < value.wheel_spin.size(); ++index)
	{
		output << "landing_gear.wheel_spin[" << index << "]="
			<< value.wheel_spin[index] << '\n';
	}
}

void write_suspension_wheel(
	std::ostringstream& output,
	std::size_t index,
	const Core::SuspensionWheelOutput& value)
{
	const std::string prefix = "suspension.wheels[" +
		std::to_string(index) + "].";
	write_vec3(output, (prefix + "acting_force").c_str(), value.acting_force);
	output << prefix << "compression=" << value.compression << '\n';
	output << prefix << "force_magnitude=" << value.force_magnitude << '\n';
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
	output << "fuel.internal_fuel=" << value.internal_fuel << '\n';
	output << "fuel.external_fuel=" << value.external_fuel << '\n';
	output << "fuel.total_fuel=" << value.total_fuel << '\n';
	output << "fuel.total_fuel_flow=" << value.total_fuel_flow << '\n';
}

void write_mass_effect(
	std::ostringstream& output,
	const Core::MassDeltaResult& value)
{
	output << "mass_effect.available=" << value.available << '\n';
	output << "mass_effect.delta.mass=" << value.delta.mass << '\n';
	write_vec3(output, "mass_effect.delta.position", value.delta.position);
	write_vec3(
		output,
		"mass_effect.delta.moment_of_inertia",
		value.delta.moment_of_inertia);
}

std::string frame_snapshot(const Core::FrameOutput& frame)
{
	std::ostringstream output;
	output << std::setprecision(kDoubleRoundTripPrecision) << std::boolalpha;
	output << "simulation_time_s=" << frame.simulation_time_s << '\n';
	write_availability(output, frame.availability);
	write_flight(output, frame.flight);
	write_vec3(output, "force_moment.force", frame.force_moment.force);
	write_vec3(output, "force_moment.moment", frame.force_moment.moment);
	write_vec3(
		output, "force_moment.center_of_mass", frame.force_moment.center_of_mass);
	for (std::size_t index = 0; index < frame.engines.size(); ++index)
	{
		write_engine(output, index, frame.engines[index]);
	}
	write_controls(output, frame.controls);
	write_landing_gear(output, frame.landing_gear);
	write_suspension(output, frame.suspension);
	write_fuel(output, frame.fuel);
	write_mass_effect(output, frame.mass_effect);
	output << "shake_amplitude=" << frame.shake_amplitude << '\n';
	return output.str();
}

void send_trajectory_commands(Core::Fck1cEfm& efm)
{
	efm.handle_command({
		Core::CommandGroup::PitchRoll, Core::CommandId::SetPitchAxis, 0.25 });
	efm.handle_command({
		Core::CommandGroup::PitchRoll, Core::CommandId::SetRollAxis, -0.2 });
	efm.handle_command({
		Core::CommandGroup::Yaw, Core::CommandId::SetYawAxis, 0.15 });
	efm.handle_command({
		Core::CommandGroup::Throttle, Core::CommandId::SetCommonThrottleAxis, 0.8 });
	efm.handle_command({
		Core::CommandGroup::LandingGear, Core::CommandId::SetGear, 0.0 });
	efm.handle_command({
		Core::CommandGroup::Airframe, Core::CommandId::SetFlapsAuto, 1.0 });
	efm.handle_command({
		Core::CommandGroup::Airframe, Core::CommandId::SetAirbrake, 1.0 });
}

std::array<
	Core::FrameOutput,
	Tests::Fck1c::kCharacterizationFrameCount> run_trajectory()
{
	Tests::Fck1c::TestAircraftConfig config = Tests::Fck1c::make_test_config();
	config.engine.fuel_consumption_rate = 3.0;
	Core::Fck1cEfm efm(config);
	(void)efm.start(Core::StartMode::HotGround);
	efm.set_internal_fuel(500.0);
	efm.set_external_fuel({ 1, 120.0, { 0.5, -0.2, 0.1 } });
	send_trajectory_commands(efm);

	Core::FrameInput input = Tests::Fck1c::make_frame_input();
	input.autopilot = {};
	std::array<
		Core::FrameOutput,
		Tests::Fck1c::kCharacterizationFrameCount> frames;
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

void expect_snapshot(
	Tests::Context& context,
	const std::string& actual,
	std::string_view expected)
{
	if (actual != expected)
	{
		std::printf(
			"Actual frame snapshot:\n%sExpected frame snapshot:\n%.*s",
			actual.c_str(),
			static_cast<int>(expected.size()),
			expected.data());
	}
	TEST_EXPECT(context, actual == expected);
}

void test_grouped_scheduler_multiframe_golden_trajectory(
	Tests::Context& context)
{
	// Phase 4 intentionally shifts the declared cross-group signals by one
	// frame and lets every System observe the current normalized DCS input.
	const auto actual = run_trajectory();
	for (std::size_t index = 0; index < actual.size(); ++index)
	{
		expect_snapshot(
			context,
			frame_snapshot(actual[index]),
			Tests::Fck1c::kCharacterizationSnapshots[index]);
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
		if (previous.landing_gear.gear_position <= kGearMidpoint &&
			current.landing_gear.gear_position > kGearMidpoint)
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
	Core::Fck1cEfm efm(Tests::Fck1c::make_test_config());
	const Core::FrameOutput start = efm.start(Core::StartMode::HotAir);
	efm.handle_command({
		Core::CommandGroup::LandingGear, Core::CommandId::SetGear, 1.0 });
	efm.handle_command({
		Core::CommandGroup::Airframe, Core::CommandId::SetFlapsAuto, 1.0 });
	Core::FrameInput input = Tests::Fck1c::make_frame_input();
	input.autopilot = {};
	const std::optional<GearCrossing> crossing =
		find_gear_midpoint_crossing(efm, input, start);
	TEST_EXPECT(context, crossing.has_value());
	if (!crossing)
	{
		return;
	}
	TEST_EXPECT_NEAR(
		context,
		crossing->at.controls.flaps_position -
			crossing->before.controls.flaps_position,
		0.0,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		crossing->at.controls.slats_position -
			crossing->before.controls.slats_position,
		0.0,
		kTolerance);
	const Core::FrameOutput next = efm.step(input);
	TEST_EXPECT_NEAR(
		context,
		next.controls.flaps_position -
			crossing->at.controls.flaps_position,
		kExpectedFlapIncrementPerFrame,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		next.controls.slats_position -
			crossing->at.controls.slats_position,
		kExpectedSlatIncrementPerFrame,
		kTolerance);
}

double expected_fuel_flow(
	const Tests::Fck1c::TestAircraftConfig& config,
	const Core::FrameOutput& frame)
{
	const double afterburner_average = 0.5 *
		(frame.engines[0].afterburner_ratio + frame.engines[1].afterburner_ratio);
	const double afterburner_factor = 1.0 + afterburner_average *
		(config.engine.afterburner.fuel_factor - 1.0);
	return config.engine.fuel_consumption_rate *
		((frame.engines[0].throttle_output +
			frame.engines[1].throttle_output + 1.0) / 3.0) *
		afterburner_factor;
}

void test_fuel_reads_previous_committed_engine_demand(
	Tests::Context& context)
{
	Tests::Fck1c::TestAircraftConfig config = Tests::Fck1c::make_test_config();
	config.engine.fuel_consumption_rate = 3.0;
	Core::Fck1cEfm efm(config);
	const Core::FrameOutput start = efm.start(Core::StartMode::HotGround);
	efm.set_internal_fuel(100.0);
	efm.handle_command({
		Core::CommandGroup::Throttle, Core::CommandId::SetCommonThrottleAxis, 1.0 });
	Core::FrameInput input;
	input.dt_s = 0.1;
	const Core::FrameOutput first = efm.step(input);
	TEST_EXPECT(context,
		first.engines[0].throttle_output != start.engines[0].throttle_output);
	TEST_EXPECT_NEAR(
		context,
		first.fuel.total_fuel_flow,
		expected_fuel_flow(config, start),
		kTolerance);
	const Core::FrameOutput second = efm.step(input);
	TEST_EXPECT_NEAR(
		context,
		second.fuel.total_fuel_flow,
		expected_fuel_flow(config, first),
		kTolerance);
}

Core::FrameOutput run_engine_shutdown_frame(
	double internal_fuel,
	double altitude_asl)
{
	Core::Fck1cEfm efm(Tests::Fck1c::make_test_config());
	(void)efm.start(Core::StartMode::HotGround);
	efm.set_internal_fuel(internal_fuel);
	efm.handle_command({
		Core::CommandGroup::Throttle,
		Core::CommandId::SetCommonThrottleAxis,
		1.0
	});
	Core::FrameInput input = Tests::Fck1c::make_frame_input();
	input.dt_s = 0.1;
	input.atmosphere.altitude_asl = altitude_asl;
	return efm.step(input);
}

void expect_shutdown_thrust_is_inhibited(
	Tests::Context& context,
	const Core::FrameOutput& frame)
{
	TEST_EXPECT(context, frame.engines[0].throttle_output > 0.0);
	TEST_EXPECT_NEAR(context, frame.engines[0].thrust_force, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, frame.engines[1].thrust_force, 0.0, kTolerance);
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
	input.body_kinematics.acceleration = { 0.0, 9.81, 0.0 };
	return input;
}

Core::FrameOutput run_ground_frame(
	Tests::Fck1c::TestAircraftConfig config,
	bool feedback_available)
{
	Core::Fck1cEfm efm(config);
	(void)efm.start(Core::StartMode::HotGround);
	efm.set_internal_fuel(100.0);
	return efm.step(make_ground_input(feedback_available));
}

struct PairedHotGroundEfms
{
	PairedHotGroundEfms()
		: subject(Tests::Fck1c::make_test_config()),
		control(Tests::Fck1c::make_test_config())
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
		with_feedback.force_moment.force.y,
		without_fallback.force_moment.force.y,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		with_feedback.force_moment.moment.z,
		without_fallback.force_moment.moment.z,
		kTolerance);
	TEST_EXPECT(context,
		without_feedback.force_moment.force.y >
			with_feedback.force_moment.force.y);
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
		damaged_frame.engines[0].thrust_force <
			control_frame.engines[0].thrust_force);
	pair.subject.repair({});
	const Core::FrameOutput repaired = pair.subject.step(input);
	const Core::FrameOutput expected = pair.control.step(input);
	TEST_EXPECT(context,
		repaired.engines[0].thrust_force >
			damaged_frame.engines[0].thrust_force);
	TEST_EXPECT(context,
		repaired.engines[0].thrust_force != expected.engines[0].thrust_force);
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
		ignored.engines[0].thrust_force,
		undamaged.engines[0].thrust_force,
		kTolerance);
}

void test_each_frame_exposes_its_mass_effect(Tests::Context& context)
{
	Tests::Fck1c::TestAircraftConfig config = Tests::Fck1c::make_test_config();
	config.engine.fuel_consumption_rate = 3.0;
	Core::Fck1cEfm efm(config);
	(void)efm.start(Core::StartMode::HotGround);
	efm.set_internal_fuel(100.0);
	Core::FrameInput input;
	input.dt_s = 0.1;
	const Core::FrameOutput first = efm.step(input);
	const Core::FrameOutput second = efm.step(input);
	TEST_EXPECT(context, first.mass_effect.available);
	TEST_EXPECT(context, second.mass_effect.available);
	TEST_EXPECT_NEAR(
		context,
		first.mass_effect.delta.mass,
		first.fuel.total_fuel_flow * input.dt_s,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		second.mass_effect.delta.mass,
		second.fuel.total_fuel_flow * input.dt_s,
		kTolerance);
}

void test_infinite_fuel_suppresses_mass_effect(Tests::Context& context)
{
	Tests::Fck1c::TestAircraftConfig config = Tests::Fck1c::make_test_config();
	config.engine.fuel_consumption_rate = 3.0;
	Core::Fck1cEfm efm(config);
	(void)efm.start(Core::StartMode::HotGround);
	efm.set_internal_fuel(100.0);
	Core::FrameInput input;
	input.dt_s = 0.1;
	TEST_EXPECT(context, efm.step(input).mass_effect.available);
	const double fuel_before = efm.internal_fuel();
	efm.set_infinite_fuel(true);
	const Core::FrameOutput unlimited = efm.step(input);
	TEST_EXPECT(context, !unlimited.mass_effect.available);
	TEST_EXPECT_NEAR(
		context, unlimited.fuel.total_fuel_flow, 0.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, efm.internal_fuel(), fuel_before, kTolerance);
}
}

void run_fck1c_efm_characterization_tests(Tests::Context& context)
{
	test_grouped_scheduler_multiframe_golden_trajectory(context);
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
