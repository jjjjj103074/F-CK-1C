#include "SystemPipelineTestFixture.h"

#include "Common/Units.h"
#include "Core/Systems/FlightControlComputer/FlightControlExecutive.h"
#include "Core/Systems/FlightControlComputer/Util/FlightStateComputation.h"

#include <cmath>

namespace
{
using namespace Core;
using namespace Core::Systems;
using namespace SystemPipelineTest;

constexpr double kTolerance = 1e-12;
constexpr double kWorldYawRad = -0.75;
constexpr double kMagneticHeadingDeg = Common::deg(1.25);
constexpr double kFccDtS = 1.0 / 64.0;
constexpr std::uint64_t kLongRunFccTicks = 4096;

Core::Systems::FlightControlComputerStepInput fcc_input()
{
	Core::Systems::FlightControlComputerStepInput input;
	input.flight_control.dt_s = kFccDtS;
	input.flight_control.observation.pressure_altitude_available = true;
	input.flight_control.observation.pressure_altitude_ft = 5000.0;
	input.flight_control.observation.indicated_airspeed_mps = 150.0;
	input.flight_control.observation.dynamic_pressure_pa = 5000.0;
	input.flight_control.observation.normal_acceleration_g = 1.0;
	return input;
}

void test_typed_magnetic_heading_is_not_world_yaw(
	Tests::Context& context)
{
	SystemPipeline pipeline(flight_setup(), {});
	FrameInput frame;
	frame.cockpit.magnetic_heading.status = {
		true, 1, ObservationInvalidReason::None
	};
	frame.cockpit.magnetic_heading.magnetic_heading_deg =
		kMagneticHeadingDeg;
	AircraftObservation aircraft;
	aircraft.world_yaw_rad = kWorldYawRad;
	const auto output = step_pipeline(pipeline, frame, aircraft);
	const auto& control = output.read(
		AircraftDataKeys::kFlightControlObservation);
	TEST_EXPECT(context, control.magnetic_heading_available);
	TEST_EXPECT_NEAR(
		context, control.magnetic_heading_deg,
		kMagneticHeadingDeg, kTolerance);
	TEST_EXPECT_NEAR(
		context,
		output.read(AircraftDataKeys::kAircraftObservation).world_yaw_rad,
		kWorldYawRad,
		kTolerance);
}

void test_pressure_altitude_never_falls_back_to_geometric_asl(
	Tests::Context& context)
{
	SystemPipeline pipeline(flight_setup(), {});
	FrameInput frame;
	AircraftObservation aircraft;
	aircraft.altitude_asl_m = 4200.0;
	auto output = step_pipeline(pipeline, frame, aircraft);
	auto control = output.read(AircraftDataKeys::kFlightControlObservation);
	TEST_EXPECT(context, !control.pressure_altitude_available);
	TEST_EXPECT_NEAR(context, control.pressure_altitude_ft, 0.0, kTolerance);
	frame.cockpit.pressure_altitude.status = {
		true, 1, ObservationInvalidReason::None
	};
	frame.cockpit.pressure_altitude.pressure_altitude_ft = 12345.0;
	output = step_pipeline(pipeline, frame, aircraft);
	control = output.read(AircraftDataKeys::kFlightControlObservation);
	TEST_EXPECT(context, control.pressure_altitude_available);
	TEST_EXPECT_NEAR(
		context, control.pressure_altitude_ft, 12345.0, kTolerance);
}

void test_pitch_shaping_runs_at_64_hz_while_roll_is_held_at_32_hz(
	Tests::Context& context)
{
	Core::Systems::FlightControlExecutive executive(
		Core::Systems::fck1c_flight_control_computer_config(),
		Core::StartMode::HotAir, {});
	auto input = fcc_input();
	(void)executive.update(input);
	input.flight_control.pilot.pitch_axis_normalized = 1.0;
	input.flight_control.pilot.roll_axis_normalized = 1.0;
	const auto result = executive.update(input);
	TEST_EXPECT(context,
		result.diagnostics.conditioned_pitch_input_normalized > 0.0);
	TEST_EXPECT_NEAR(context,
		result.diagnostics.conditioned_roll_input_normalized,
		0.0, kTolerance);
	TEST_EXPECT(context,
		result.diagnostics.pilot_shaping_update_count == 1);
}

void test_fcc_subrates_keep_fixed_phase_without_long_run_drift(
	Tests::Context& context)
{
	Core::Systems::FlightControlExecutive executive(
		Core::Systems::fck1c_flight_control_computer_config(),
		Core::StartMode::HotAir, {});
	const auto input = fcc_input();
	for (std::uint64_t tick = 0; tick < kLongRunFccTicks; ++tick)
		(void)executive.update(input);
	const auto& snapshot = executive.result().diagnostics;
	TEST_EXPECT(context, snapshot.flight_control_tick == 4095);
	TEST_EXPECT(context, snapshot.pilot_shaping_update_count == 2048);
	TEST_EXPECT(context, snapshot.pilot_shaping_last_update_tick == 4094);
	TEST_EXPECT(context, snapshot.pilot_shaping_age_ticks == 1);
	TEST_EXPECT(context, snapshot.gain_schedule_update_count == 256);
	TEST_EXPECT(context, snapshot.gain_schedule_last_update_tick == 4080);
	TEST_EXPECT(context, snapshot.gain_schedule_age_ticks == 15);
}

void test_computed_state_derives_flight_path_angle(Tests::Context& context)
{
	::Systems::ManagedFlightControlSignals signals;
	signals.observation.indicated_airspeed_mps = 150.0;
	signals.observation.vertical_speed_ft_s = Common::feet(15.0);
	const auto state = Core::Systems::compute_flight_state(signals);
	TEST_EXPECT(context, state.flight_path_angle_available);
	TEST_EXPECT_NEAR(
		context, state.flight_path_angle_rad, std::asin(0.1), kTolerance);
}
}

void run_flight_control_observation_pipeline_tests(Tests::Context& context)
{
	test_typed_magnetic_heading_is_not_world_yaw(context);
	test_pressure_altitude_never_falls_back_to_geometric_asl(context);
	test_pitch_shaping_runs_at_64_hz_while_roll_is_held_at_32_hz(context);
	test_fcc_subrates_keep_fixed_phase_without_long_run_drift(context);
	test_computed_state_derives_flight_path_angle(context);
}
