#include "TestHarness.h"
#include "Fck1cEfmTestFixture.h"

#include "Core/Fck1cEfm.h"
#include "Core/Systems/SystemPipeline.h"

#include <utility>

namespace
{
constexpr double kTolerance = 1e-9;
constexpr double kSchedulerAdvanceS = 0.02;

using Tests::Fck1c::make_frame_input;
using Tests::Fck1c::make_test_config;

void test_fcc_override_preserves_defaults_and_factory_owns_config(
	Tests::Context& context)
{
	Core::Systems::SystemEntry entry;
	{
		// 覆蓋來源離開作用範圍後，延後建立的工廠仍須保有完整設定。
		std::vector<Tests::Fck1c::FlightControlComputerConfigOverride>
			overrides;
		overrides.emplace_back(
			[](Core::Systems::FlightControlComputerConfigDraft& config)
			{
				config.mode_and_gain.g_limiter_override.available = true;
			});
		entry = Tests::Fck1c::make_test_flight_control_computer_entry(
			overrides);
	}
	const Core::Systems::FlightSetupContext setup = {
		Core::StartMode::HotGround, {}, {}, Tests::disabled_debug_telemetry()
	};
	auto catalog = Core::Systems::load_generated_system_catalog();
	Tests::Fck1c::replace_system_entry(catalog, std::move(entry));
	Core::Systems::SystemPipeline pipeline(setup, std::move(catalog));
	const auto initial = pipeline.snapshot();
	const auto& diagnostics = initial.read(
		Core::AircraftDataKeys::kFlightControlComputerSnapshot);
	TEST_EXPECT(context,
		diagnostics.developer_g_limiter_override_available);
	TEST_EXPECT(context,
		diagnostics.developer_direct_control_law_active ==
		Core::Systems::fck1c_flight_control_computer_config().
			values.flight_control_output.developer_direct_control_law);
}

void test_fcc_command_registration_follows_instance_config(
	Tests::Context& context)
{
	using Core::Systems::DispatchResult;
	const Core::Systems::FlightSetupContext setup = {
		Core::StartMode::HotGround, {}, {}, Tests::disabled_debug_telemetry()
	};
	Core::Systems::SystemPipeline standard(
		setup, Core::Systems::load_generated_system_catalog());
	TEST_EXPECT(context, standard.send({ Core::CommandId::ToggleFbwCat, 1.0 }) ==
		DispatchResult::Handled);
	TEST_EXPECT(context, standard.send({ Core::CommandId::EngageAutopilot, 1.0 }) ==
		DispatchResult::Handled);
	TEST_EXPECT(context, standard.send({ Core::CommandId::EngageAutoThrottle, 1.0 }) ==
		DispatchResult::Unhandled);

	std::vector<Tests::Fck1c::FlightControlComputerConfigOverride> overrides;
	overrides.emplace_back([](Core::Systems::FlightControlComputerConfigDraft& config)
	{
		config.automatic_flight_control.experimental_auto_throttle_available = true;
	});
	auto catalog = Core::Systems::load_generated_system_catalog();
	Tests::Fck1c::replace_system_entry(catalog,
		Tests::Fck1c::make_test_flight_control_computer_entry(overrides));
	Core::Systems::SystemPipeline experimental(setup, std::move(catalog));
	TEST_EXPECT(context, experimental.send({ Core::CommandId::EngageAutoThrottle, 1.0 }) ==
		DispatchResult::Handled);
}

Core::FrameInput make_airborne_frame_input()
{
	Core::FrameInput input = make_frame_input();
	input.suspension = {};
	return input;
}

void test_automatic_flight_commands_drive_outputs(
	Tests::Context& context)
{
	auto config = make_test_config();
	config.flight_control_computer_overrides.emplace_back(
		[](Core::Systems::FlightControlComputerConfigDraft& flight_control)
		{
			flight_control.automatic_flight_control.
				experimental_auto_throttle_available =
				true;
		});
	Core::Fck1cEfm efm(config, Tests::disabled_debug_telemetry());
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
	TEST_EXPECT_NEAR(
		context, output.controls.pitch_input_normalized, 0.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, output.controls.roll_input_normalized, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, output.engines[0].throttle_input_normalized,
		automatic_throttle, kTolerance);
	output = efm.step(input);
	output = efm.step(input);
	TEST_EXPECT(context, output.cockpit.automatic_flight_control.master_engaged);
	TEST_EXPECT(context, output.controls.symmetric_stabilator_position_rad != 0.0);
	TEST_EXPECT(context, output.controls.differential_flaperon_position_rad != 0.0);
	TEST_EXPECT_NEAR(context, output.engines[0].throttle_input_normalized,
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
	TEST_EXPECT_NEAR(context, actual.controls.symmetric_stabilator_position_rad,
		expected.controls.symmetric_stabilator_position_rad, kTolerance);
	TEST_EXPECT_NEAR(context, actual.controls.differential_flaperon_position_rad,
		expected.controls.differential_flaperon_position_rad, kTolerance);
}

void test_available_developer_g_override_is_visible(
	Tests::Context& context)
{
	auto config = make_test_config();
	config.flight_control_computer_overrides.emplace_back(
		[](Core::Systems::FlightControlComputerConfigDraft& flight_control)
		{
			flight_control.mode_and_gain.g_limiter_override.available = true;
		});
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
	TEST_EXPECT(context, output.engines[0].thrust_force_n > 0.0);
	efm.handle_command({ Core::CommandId::EnableThrustCutTest, 1.0 });
	output = efm.step(input);
	TEST_EXPECT(context, output.propulsion_diagnostics.thrust_cut_requested);
	TEST_EXPECT_NEAR(
		context, output.engines[0].thrust_force_n, 0.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, output.engines[1].thrust_force_n, 0.0, kTolerance);
	efm.handle_command({ Core::CommandId::DisableThrustCutTest, 1.0 });
	output = efm.step(input);
	TEST_EXPECT(context, !output.propulsion_diagnostics.thrust_cut_requested);
	TEST_EXPECT(context, output.engines[0].thrust_force_n > 0.0);
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
	TEST_EXPECT_NEAR(
		context, output.controls.pitch_input_normalized, 0.3, kTolerance);
	TEST_EXPECT(context, output.engines[0].thrust_force_n > 0.0);
	TEST_EXPECT(context, output.engines[1].thrust_force_n > 0.0);
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
	test_fcc_override_preserves_defaults_and_factory_owns_config(context);
	test_fcc_command_registration_follows_instance_config(context);
	test_automatic_flight_commands_drive_outputs(context);
	test_unavailable_developer_g_override_cannot_affect_flight(context);
	test_available_developer_g_override_is_visible(context);
	test_propulsion_diagnostics_commands_drive_outputs(context);
	test_neutral_cockpit_input_completes_step(context);
	test_damage_returns_immediate_result(context);
}
