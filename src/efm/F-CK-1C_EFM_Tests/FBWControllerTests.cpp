#include "TestHarness.h"

#include "Core/Systems/FlightControlComputer/ControlLaws/ControlLaws.h"
#include "Core/Systems/FlightControlComputer/ControlLaws/ConfigurationAndMode.h"
#include "Core/Systems/FlightControlComputer/ControlLaws/ControlLawMath.h"
#include "Core/Systems/FlightControlComputer/ControlLaws/PilotCommandLaw.h"
#include "Core/Systems/FlightControlComputer/InputSignalManagement.h"

#include <cmath>

namespace
{
constexpr double kSnapshotTolerance = 1e-9;
constexpr int kInitialSnapshotFrameCount = 1;
constexpr int kReferenceSnapshotFrameCount = 50;
constexpr int kModeTransitionFrameCount = 50;
constexpr int kHoldEngagementFrameCount = 30;
constexpr int kAoaDegradeFrameCount = 20;

Core::Systems::RawFlightControlInput make_reference_input()
{
	Core::Systems::RawFlightControlInput input;
	input.dt_s = 0.01;
	input.alpha_limit_deg = 20.0;
	input.observation.dynamic_pressure_pa = 5000.0;
	input.observation.roll_rad = 0.1;
	input.observation.pitch_rad = 0.05;
	input.observation.roll_rate_rad_s = 0.02;
	input.observation.pitch_rate_rad_s = -0.03;
	input.observation.yaw_rate_rad_s = 0.01;
	input.observation.alpha_deg = 3.0;
	input.observation.beta_deg = 1.0;
	input.observation.indicated_airspeed_mps = 150.0;
	input.observation.mach = 0.5;
	input.observation.normal_acceleration_g = 1.0;
	input.pilot.roll_axis_normalized = 0.2;
	input.pilot.pitch_axis_normalized = -0.15;
	input.pilot.yaw_axis_normalized = 0.1;
	return input;
}

void carry_output(
	Core::Systems::RawFlightControlInput& input,
	const Systems::FlightControlLawResult& output)
{
	input.actuator.elevator.normalized_position =
		output.surface_demand.elevator_command_normalized;
	input.actuator.aileron.normalized_position =
		output.surface_demand.aileron_command_normalized;
	input.actuator.rudder.normalized_position =
		output.surface_demand.rudder_command_normalized;
}

Systems::FlightControlLawStepInput make_manual_request(
	const Systems::ConditionedFlightControlInput& flight,
	const Systems::FBWControllerConfig& config,
	const Systems::FBWControllerState& state)
{
	const Systems::FlightControlConfiguration configuration =
		Systems::make_flight_control_configuration(
			config,
			{ flight.cat_mode_blend, flight.dynamic_pressure_pa,
				flight.alpha_limit_deg, state.g_limiter_override });
	return {
		flight,
		Systems::make_pilot_maneuver_reference(
			{ flight, config, configuration.cat, configuration.gains,
				configuration.envelope }),
		configuration
	};
}

struct FBWTestRig
{
	FBWTestRig() : input_signals(config)
	{
		reset();
	}

	void reset()
	{
		state = Systems::FBWControllerState();
		Systems::reset_fbw_state(
			state,
			{ input.observation.roll_rad,
				input.observation.pitch_rad,
				input.observation.normal_acceleration_g });
		output = Systems::FlightControlLawResult();
	}

	void advance(int frame_count)
	{
		for (int frame = 0; frame < frame_count; ++frame)
		{
			carry_output(input, output);
			const auto conditioned = input_signals.condition(
				input, state.mode_target);
			output = Systems::update_fbw_controller(
				state, config, make_manual_request(conditioned, config, state));
		}
	}

	Systems::FBWControllerConfig config;
	Core::Systems::InputSignalManagement input_signals;
	Core::Systems::RawFlightControlInput input = make_reference_input();
	Systems::FBWControllerState state;
	Systems::FlightControlLawResult output;
};

void expect_output(
	Tests::Context& context,
	const Systems::FlightControlLawResult& output,
	const Systems::FlightControlLawResult& expected)
{
	TEST_EXPECT_NEAR(
		context,
		output.surface_demand.elevator_command_normalized,
		expected.surface_demand.elevator_command_normalized,
		kSnapshotTolerance);
	TEST_EXPECT_NEAR(
		context,
		output.surface_demand.aileron_command_normalized,
		expected.surface_demand.aileron_command_normalized,
		kSnapshotTolerance);
	TEST_EXPECT_NEAR(
		context,
		output.surface_demand.rudder_command_normalized,
		expected.surface_demand.rudder_command_normalized,
		kSnapshotTolerance);
}

void test_reference_frame_snapshots(Tests::Context& context)
{
	FBWTestRig rig;
	rig.advance(kInitialSnapshotFrameCount);
	TEST_EXPECT(
		context,
		std::abs(rig.output.surface_demand.aileron_command_normalized) > 0.0);
	TEST_EXPECT(
		context,
		std::abs(rig.output.surface_demand.rudder_command_normalized) > 0.0);
	TEST_EXPECT(context, !rig.state.actuator_sat);
	rig.advance(kReferenceSnapshotFrameCount - kInitialSnapshotFrameCount);
	TEST_EXPECT(context, std::isfinite(rig.state.p_cmd));
	TEST_EXPECT(context, std::isfinite(rig.state.q_cmd));
	TEST_EXPECT(context, std::isfinite(rig.state.r_cmd));
	TEST_EXPECT(
		context,
		std::abs(rig.output.surface_demand.elevator_command_normalized) <= 1.0);
}

void test_hold_snapshot(Tests::Context& context)
{
	FBWTestRig rig;
	rig.input.pilot.roll_axis_normalized = 0.0;
	rig.input.pilot.pitch_axis_normalized = 0.0;
	rig.input.pilot.yaw_axis_normalized = 0.0;
	rig.reset();
	rig.advance(kHoldEngagementFrameCount);
	TEST_EXPECT(context, rig.state.control_state == Systems::FBW_STATE_HOLD);
	TEST_EXPECT(context, rig.state.hold_active);
	TEST_EXPECT(context, std::isfinite(rig.state.q_cmd));
}

void test_direct_mode_snapshot(Tests::Context& context)
{
	Systems::FBWControllerConfig config;
	Systems::FBWControllerState state;
	state.enabled = false;
	Systems::ConditionedFlightControlInput input;
	input.dt_s = 0.01;
	input.pilot_roll_raw_normalized = -0.2;
	input.pilot_pitch_raw_normalized = 0.5;
	input.pilot_yaw_raw_normalized = -0.15;
	input.elevator_position_normalized = 0.1;
	input.aileron_position_normalized = -0.2;
	input.rudder_position_normalized = 0.3;
	const auto output = Systems::update_fbw_controller(
		state, config, { input, {} });
	expect_output(context, output, { 0.1125, -0.2, 0.288 });
}

void test_fbw_commands(Tests::Context& context)
{
	Systems::FBWControllerState state;
	Systems::toggle_fbw_cat_mode(state, false);
	TEST_EXPECT(context, state.mode_target == Systems::FBW_CAT1);
	Systems::toggle_fbw_cat_mode(state, true);
	TEST_EXPECT(context, state.mode_target == Systems::FBW_CAT3);
	Systems::set_fbw_cat_mode(state, Systems::FBW_CAT1);
	TEST_EXPECT(context, state.mode_target == Systems::FBW_CAT1);
	Systems::set_fbw_g_limiter_override(state, true);
	TEST_EXPECT(context, state.g_limiter_override);
	Systems::toggle_fbw_g_limiter_override(state, true);
	TEST_EXPECT(context, !state.g_limiter_override);
}

void test_actuator_feedback_is_consumed(Tests::Context& context)
{
	FBWTestRig rig;
	rig.input.actuator.any_saturated = true;
	rig.advance(kInitialSnapshotFrameCount);
	TEST_EXPECT(context, rig.state.actuator_sat);
}

void test_fbw_reset(Tests::Context& context)
{
	Systems::FBWControllerState state;
	state.control_state = Systems::FBW_STATE_DEGRADE;
	state.int_p = 0.5;
	state.actuator_sat = true;
	state.throttle_cmd_left = 0.7;
	Systems::reset_fbw_state(state, { 0.2, -0.1, 1.3 });
	TEST_EXPECT(context, state.control_state == Systems::FBW_STATE_RATE);
	TEST_EXPECT_NEAR(context, state.phi_ref, 0.2, kSnapshotTolerance);
	TEST_EXPECT_NEAR(context, state.theta_ref, -0.1, kSnapshotTolerance);
	TEST_EXPECT_NEAR(context, state.int_p, 0.0, kSnapshotTolerance);
	TEST_EXPECT(context, !state.actuator_sat);
	TEST_EXPECT_NEAR(context, state.throttle_cmd_left, 0.7, kSnapshotTolerance);
	Systems::reset_fbw_throttle_interface(state);
	TEST_EXPECT_NEAR(context, state.throttle_cmd_left, 0.0, kSnapshotTolerance);
}

void test_limiters_and_actuator_bounds(Tests::Context& context)
{
	Systems::FBWControllerConfig config;
	config.cat1.command_shape_tau = 0.0;
	config.cat1.command_shape_rate = 1000.0;
	config.cat1.stick_expo = 0.0;
	config.cat1.aoa_soft_deg = 1.0;
	config.cat3.aoa_soft_deg = 1.0;
	config.cat1.g_soft = 1.1;
	config.cat1.g_hard = 3.0;
	Core::Systems::RawFlightControlInput raw = make_reference_input();
	raw.pilot.pitch_axis_normalized = 0.5;
	raw.observation.alpha_deg = 30.0;
	Core::Systems::InputSignalManagement input_signals(config);
	const Systems::ConditionedFlightControlInput input =
		input_signals.condition(raw, Systems::FBW_CAT1);
	Systems::FBWControllerState state;
	Systems::reset_fbw_state(
		state,
		{ input.roll_attitude_rad,
			input.pitch_attitude_rad,
			input.normal_acceleration_g });
	const auto output = Systems::update_fbw_controller(
		state, config, make_manual_request(input, config, state));
	TEST_EXPECT(context, state.aoa_limit_active);
	TEST_EXPECT(context, state.g_limit_active);
	TEST_EXPECT(
		context,
		output.surface_demand.elevator_command_normalized >= -1.0 &&
			output.surface_demand.elevator_command_normalized <= 1.0);
	TEST_EXPECT(
		context,
		output.surface_demand.aileron_command_normalized >= -1.0 &&
			output.surface_demand.aileron_command_normalized <= 1.0);
	TEST_EXPECT(
		context,
		output.surface_demand.rudder_command_normalized >= -1.0 &&
			output.surface_demand.rudder_command_normalized <= 1.0);
}

void test_cat_transition_and_hold_degrade(Tests::Context& context)
{
	FBWTestRig rig;
	Systems::set_fbw_cat_mode(rig.state, Systems::FBW_CAT3);
	rig.advance(kModeTransitionFrameCount);
	TEST_EXPECT(context, rig.state.mode_blend > 0.5);
	rig.input.pilot.roll_axis_normalized = 0.0;
	rig.input.pilot.pitch_axis_normalized = 0.0;
	rig.advance(kHoldEngagementFrameCount);
	TEST_EXPECT(context, rig.state.control_state == Systems::FBW_STATE_HOLD);
	rig.input.observation.alpha_deg = 30.0;
	rig.advance(kAoaDegradeFrameCount);
	TEST_EXPECT(context, rig.state.control_state == Systems::FBW_STATE_DEGRADE);
	TEST_EXPECT(context, rig.state.hold_exit_reason == Systems::FBW_HOLD_EXIT_AOA);
}
}

void run_fbw_controller_tests(Tests::Context& context)
{
	test_reference_frame_snapshots(context);
	test_hold_snapshot(context);
	test_direct_mode_snapshot(context);
	test_fbw_commands(context);
	test_actuator_feedback_is_consumed(context);
	test_fbw_reset(context);
	test_limiters_and_actuator_bounds(context);
	test_cat_transition_and_hold_degrade(context);
}
