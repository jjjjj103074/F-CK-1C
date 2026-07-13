#include "TestHarness.h"

#include "Systems/FBWController.h"

namespace
{
constexpr double kSnapshotTolerance = 1e-12;
constexpr int kInitialSnapshotFrameCount = 1;
constexpr int kReferenceSnapshotFrameCount = 50;
constexpr int kModeTransitionFrameCount = 50;
constexpr int kHoldEngagementFrameCount = 30;
constexpr int kAoaDegradeFrameCount = 20;

Systems::FBWControllerInput make_reference_input()
{
	Systems::FBWControllerInput input;
	input.dt = 0.01;
	input.qbar = 5000.0;
	input.alpha_limit_deg = 20.0;
	input.roll = 0.1;
	input.pitch = 0.05;
	input.roll_rate = 0.02;
	input.pitch_rate = -0.03;
	input.yaw_rate = 0.01;
	input.alpha = 3.0;
	input.beta = 1.0;
	input.speed_scalar = 150.0;
	input.mach = 0.5;
	input.g = 1.0;
	input.roll_input = 0.2;
	input.pitch_input = -0.15;
	input.yaw_input = 0.1;
	return input;
}

void carry_output(
	Systems::FBWControllerInput& input,
	const Systems::FBWControllerOutput& output)
{
	input.elevator_command = output.elevator_command;
	input.aileron_command = output.aileron_command;
	input.rudder_command = output.rudder_command;
}

struct FBWTestRig
{
	FBWTestRig()
	{
		reset();
	}

	void reset()
	{
		state = Systems::FBWControllerState();
		Systems::reset_fbw_state(
			state,
			{ input.roll, input.pitch, input.alpha, input.g });
		output = Systems::FBWControllerOutput();
	}

	void advance(int frame_count)
	{
		for (int frame = 0; frame < frame_count; ++frame)
		{
			carry_output(input, output);
			output = Systems::update_fbw_controller(state, config, input);
		}
	}

	Systems::FBWControllerConfig config;
	Systems::FBWControllerInput input = make_reference_input();
	Systems::FBWControllerState state;
	Systems::FBWControllerOutput output;
};

void expect_output(
	Tests::Context& context,
	const Systems::FBWControllerOutput& output,
	const Systems::FBWControllerOutput& expected)
{
	TEST_EXPECT_NEAR(context, output.elevator_command, expected.elevator_command, kSnapshotTolerance);
	TEST_EXPECT_NEAR(context, output.aileron_command, expected.aileron_command, kSnapshotTolerance);
	TEST_EXPECT_NEAR(context, output.rudder_command, expected.rudder_command, kSnapshotTolerance);
}

void test_reference_frame_snapshots(Tests::Context& context)
{
	FBWTestRig rig;
	rig.advance(kInitialSnapshotFrameCount);
	expect_output(context, rig.output,
		{ 0.0023222816610468443, 0.0083333333333333332, 0.0017862844650569911 });
	TEST_EXPECT(context, rig.state.actuator_sat);
	rig.advance(kReferenceSnapshotFrameCount - kInitialSnapshotFrameCount);
	expect_output(context, rig.output,
		{ -0.077868133088307326, 0.43456274515860627, 0.077430492540090767 });
	TEST_EXPECT_NEAR(context, rig.state.p_cmd, 0.63520036385081835, kSnapshotTolerance);
	TEST_EXPECT_NEAR(context, rig.state.q_cmd, -0.11632423571852148, kSnapshotTolerance);
	TEST_EXPECT_NEAR(context, rig.state.r_cmd, 0.1112729717086177, kSnapshotTolerance);
}

void test_hold_snapshot(Tests::Context& context)
{
	FBWTestRig rig;
	rig.input.roll_input = 0.0;
	rig.input.pitch_input = 0.0;
	rig.input.yaw_input = 0.0;
	rig.reset();
	rig.advance(kHoldEngagementFrameCount);
	expect_output(context, rig.output, { 0.028528760570, -0.019599302642, -0.022718110918 });
	TEST_EXPECT(context, rig.state.control_state == Systems::FBW_STATE_HOLD);
	TEST_EXPECT(context, rig.state.hold_active);
}

void test_direct_mode_snapshot(Tests::Context& context)
{
	Systems::FBWControllerConfig config;
	Systems::FBWControllerState state;
	state.enabled = false;
	Systems::FBWControllerInput input;
	input.dt = 0.01;
	input.roll_input = -0.25;
	input.roll_trim = 0.05;
	input.pitch_input = 0.4;
	input.pitch_trim = 0.1;
	input.yaw_input = -0.2;
	input.yaw_trim = 0.05;
	input.elevator_command = 0.1;
	input.aileron_command = -0.2;
	input.rudder_command = 0.3;
	const auto output = Systems::update_fbw_controller(state, config, input);
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

void test_fbw_reset(Tests::Context& context)
{
	Systems::FBWControllerState state;
	state.control_state = Systems::FBW_STATE_DEGRADE;
	state.int_p = 0.5;
	state.actuator_sat = true;
	state.throttle_cmd_left = 0.7;
	Systems::reset_fbw_state(state, { 0.2, -0.1, 4.0, 1.3 });
	TEST_EXPECT(context, state.control_state == Systems::FBW_STATE_RATE);
	TEST_EXPECT_NEAR(context, state.phi_ref, 0.2, kSnapshotTolerance);
	TEST_EXPECT_NEAR(context, state.theta_ref, -0.1, kSnapshotTolerance);
	TEST_EXPECT_NEAR(context, state.alpha_trim_deg, 4.0, kSnapshotTolerance);
	TEST_EXPECT_NEAR(context, state.nz_trim_g, 1.3, kSnapshotTolerance);
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
	config.cat1.g_soft = 1.1;
	config.cat1.g_hard = 3.0;
	config.alpha_cmd_per_stick_deg = 100.0;
	Systems::FBWControllerInput input = make_reference_input();
	input.pitch_input = 0.5;
	Systems::FBWControllerState state;
	Systems::reset_fbw_state(state, { input.roll, input.pitch, input.alpha, input.g });
	const auto output = Systems::update_fbw_controller(state, config, input);
	TEST_EXPECT(context, state.aoa_limit_active);
	TEST_EXPECT(context, state.g_limit_active);
	TEST_EXPECT(context, output.elevator_command >= -1.0 && output.elevator_command <= 1.0);
	TEST_EXPECT(context, output.aileron_command >= -1.0 && output.aileron_command <= 1.0);
	TEST_EXPECT(context, output.rudder_command >= -1.0 && output.rudder_command <= 1.0);
}

void test_cat_transition_and_hold_degrade(Tests::Context& context)
{
	FBWTestRig rig;
	Systems::set_fbw_cat_mode(rig.state, Systems::FBW_CAT3);
	rig.advance(kModeTransitionFrameCount);
	TEST_EXPECT(context, rig.state.mode_blend > 0.5);
	rig.input.roll_input = 0.0;
	rig.input.pitch_input = 0.0;
	rig.advance(kHoldEngagementFrameCount);
	TEST_EXPECT(context, rig.state.control_state == Systems::FBW_STATE_HOLD);
	rig.input.alpha = 30.0;
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
	test_fbw_reset(context);
	test_limiters_and_actuator_bounds(context);
	test_cat_transition_and_hold_degrade(context);
}
