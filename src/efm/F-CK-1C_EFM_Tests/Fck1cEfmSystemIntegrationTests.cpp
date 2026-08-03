#include "TestHarness.h"
#include "Fck1cEfmTestFixture.h"

#include "Core/Fck1cEfm.h"

namespace
{
constexpr double kTolerance = 1e-9;
constexpr double kSchedulerAdvanceS = 0.02;

using Tests::Fck1c::make_frame_input;
using Tests::Fck1c::make_test_config;

Core::FrameInput make_airborne_frame_input()
{
	Core::FrameInput input = make_frame_input();
	input.suspension = {};
	return input;
}

void test_automatic_flight_commands_drive_outputs(
	Tests::Context& context)
{
	Core::Fck1cEfm efm(
		make_test_config(), Tests::disabled_debug_telemetry());
	(void)efm.start(Core::StartMode::HotAir);
	efm.set_internal_fuel(100.0);
	const Core::FrameInput input = make_airborne_frame_input();
	(void)efm.step(input);
	efm.handle_command({ Core::CommandId::EngageAutopilot, 1.0 });
	efm.handle_command({ Core::CommandId::EngageAutoThrottle, 1.0 });
	Core::FrameOutput output = efm.step(input);
	const auto first_afcs = output.cockpit.automatic_flight_control;
	const double automatic_throttle = first_afcs.throttle_command_normalized;
	TEST_EXPECT(context, first_afcs.master_engaged);
	TEST_EXPECT(context, first_afcs.auto_throttle_engaged);
	TEST_EXPECT_NEAR(context, output.controls.pitch_input, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, output.controls.roll_input, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, output.engines[0].throttle_input,
		automatic_throttle, kTolerance);
	output = efm.step(input);
	output = efm.step(input);
	TEST_EXPECT(context, output.cockpit.automatic_flight_control.master_engaged);
	TEST_EXPECT(context, output.controls.elevator_command != 0.0);
	TEST_EXPECT(context, output.controls.aileron_command != 0.0);
	TEST_EXPECT_NEAR(context, output.engines[0].throttle_input,
		automatic_throttle, kTolerance);
}

void test_unavailable_developer_g_override_cannot_affect_flight(
	Tests::Context& context)
{
	Core::Fck1cEfm baseline(
		make_test_config(), Tests::disabled_debug_telemetry());
	Core::Fck1cEfm unavailable(
		make_test_config(), Tests::disabled_debug_telemetry());
	(void)baseline.start(Core::StartMode::HotAir);
	(void)unavailable.start(Core::StartMode::HotAir);
	unavailable.handle_command({ Core::CommandId::SetGLimiterOverride, 1.0 });
	const Core::FrameOutput expected =
		baseline.step(make_airborne_frame_input());
	const Core::FrameOutput actual =
		unavailable.step(make_airborne_frame_input());
	TEST_EXPECT(context, !actual.cockpit.flight_control_computer.
		developer_g_limiter_override_available);
	TEST_EXPECT(context, !actual.cockpit.flight_control_computer.
		developer_g_limiter_override_active);
	TEST_EXPECT_NEAR(context, actual.controls.elevator_command,
		expected.controls.elevator_command, kTolerance);
	TEST_EXPECT_NEAR(context, actual.controls.aileron_command,
		expected.controls.aileron_command, kTolerance);
}

void test_available_developer_g_override_is_visible(
	Tests::Context& context)
{
	auto config = make_test_config();
	config.flight_control_computer.
		developer_g_limiter_override_available = true;
	Core::Fck1cEfm efm(config, Tests::disabled_debug_telemetry());
	(void)efm.start(Core::StartMode::HotAir);
	efm.handle_command({ Core::CommandId::SetGLimiterOverride, 1.0 });
	const Core::FrameOutput output =
		efm.step(make_airborne_frame_input());
	TEST_EXPECT(context, output.cockpit.flight_control_computer.status.available);
	TEST_EXPECT(context, output.cockpit.flight_control_computer.
		developer_g_limiter_override_available);
	TEST_EXPECT(context, output.cockpit.flight_control_computer.
		developer_g_limiter_override_active);
}

void test_propulsion_diagnostics_commands_drive_outputs(
	Tests::Context& context)
{
	Core::Fck1cEfm efm(
		make_test_config(), Tests::disabled_debug_telemetry());
	(void)efm.start(Core::StartMode::HotAir);
	efm.set_internal_fuel(100.0);
	const Core::FrameInput input = make_airborne_frame_input();
	Core::FrameOutput output = efm.step(input);
	TEST_EXPECT(context, output.engines[0].thrust_force > 0.0);
	efm.handle_command({ Core::CommandId::EnableThrustCutTest, 1.0 });
	output = efm.step(input);
	TEST_EXPECT(context, output.propulsion_diagnostics.thrust_cut_requested);
	TEST_EXPECT_NEAR(context, output.engines[0].thrust_force, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, output.engines[1].thrust_force, 0.0, kTolerance);
	efm.handle_command({ Core::CommandId::DisableThrustCutTest, 1.0 });
	output = efm.step(input);
	TEST_EXPECT(context, !output.propulsion_diagnostics.thrust_cut_requested);
	TEST_EXPECT(context, output.engines[0].thrust_force > 0.0);
}

void test_neutral_cockpit_input_completes_step(Tests::Context& context)
{
	Core::Fck1cEfm efm(
		make_test_config(), Tests::disabled_debug_telemetry());
	(void)efm.start(Core::StartMode::HotGround);
	efm.set_internal_fuel(100.0);
	efm.handle_command({ Core::CommandId::SetPitchAxis, 0.3 });
	Core::FrameInput input;
	input.dt_s = kSchedulerAdvanceS;
	const Core::FrameOutput output = efm.step(input);
	TEST_EXPECT_NEAR(context, output.simulation_time_s,
		kSchedulerAdvanceS, kTolerance);
	TEST_EXPECT_NEAR(context, output.controls.pitch_input, 0.3, kTolerance);
	TEST_EXPECT(context, output.engines[0].thrust_force > 0.0);
	TEST_EXPECT(context, output.engines[1].thrust_force > 0.0);
}

void test_damage_returns_immediate_result(Tests::Context& context)
{
	Core::Fck1cEfm efm(
		make_test_config(), Tests::disabled_debug_telemetry());
	const Core::DamageEvent damage = { Core::DamageArea::LeftWing, 0, 0.2 };
	TEST_EXPECT(context, !efm.apply_damage(damage).invincible);
	efm.set_invincible(true);
	TEST_EXPECT(context, efm.apply_damage(damage).invincible);
}
}

void run_fck1c_efm_system_integration_tests(Tests::Context& context)
{
	test_automatic_flight_commands_drive_outputs(context);
	test_unavailable_developer_g_override_cannot_affect_flight(context);
	test_available_developer_g_override_is_visible(context);
	test_propulsion_diagnostics_commands_drive_outputs(context);
	test_neutral_cockpit_input_completes_step(context);
	test_damage_returns_immediate_result(context);
}
