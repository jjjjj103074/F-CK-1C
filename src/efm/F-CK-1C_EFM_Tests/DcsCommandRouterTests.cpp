#include "TestHarness.h"

#include "DcsBridge/DcsCommandRouter.h"
#include "DcsIds/Commands.h"

namespace
{
constexpr double kTolerance = 1e-6;

void test_primary_controls(Tests::Context& context)
{
	Core::Fck1cEfmSystems systems;
	DcsBridge::route_command(systems, DcsIds::Commands::JoystickPitch, 0.4f);
	DcsBridge::route_command(systems, DcsIds::Commands::JoystickRoll, -0.3f);
	DcsBridge::route_command(systems, DcsIds::Commands::PedalYaw, 0.2f);
	TEST_EXPECT_NEAR(context, systems.primary_controls.pitch.input, 0.4, kTolerance);
	TEST_EXPECT_NEAR(context, systems.primary_controls.roll.input, -0.3, kTolerance);
	TEST_EXPECT_NEAR(context, systems.primary_controls.yaw.input, -0.2, kTolerance);

	DcsBridge::route_command(systems, DcsIds::Commands::TrimUp, 1.0f);
	DcsBridge::route_command(systems, DcsIds::Commands::TrimLeft, 1.0f);
	DcsBridge::route_command(systems, DcsIds::Commands::RudderTrimRight, 1.0f);
	TEST_EXPECT_NEAR(context, systems.primary_controls.pitch.trim, 0.0015, kTolerance);
	TEST_EXPECT_NEAR(context, systems.primary_controls.roll.trim, -0.001, kTolerance);
	TEST_EXPECT_NEAR(context, systems.primary_controls.yaw.trim, -0.001, kTolerance);
}

void test_fbw_and_engine_commands(Tests::Context& context)
{
	Core::Fck1cEfmSystems systems;
	DcsBridge::route_command(systems, DcsIds::Commands::FBWCat3, 1.0f);
	DcsBridge::route_command(systems, DcsIds::Commands::FBWGLimiterOverride, 1.0f);
	DcsBridge::route_command(systems, DcsIds::Commands::EnginesOn, 1.0f);
	TEST_EXPECT(context, systems.fbw.mode_target == Systems::FBW_CAT3);
	TEST_EXPECT(context, systems.fbw.g_limiter_override);
	TEST_EXPECT(context, systems.engines.left.switch_on);
	TEST_EXPECT(context, systems.engines.right.switch_on);

	DcsBridge::route_command(systems, DcsIds::Commands::LeftEngineOff, 1.0f);
	TEST_EXPECT(context, !systems.engines.left.switch_on);
	TEST_EXPECT(context, systems.engines.right.switch_on);
}

void test_throttle_and_airframe_commands(Tests::Context& context)
{
	Core::Fck1cEfmSystems systems;
	DcsBridge::route_command(systems, DcsIds::Commands::ThrottleAxis, -1.0f);
	TEST_EXPECT_NEAR(context, systems.throttle_inputs.left.axis_cmd, 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, systems.throttle_inputs.right.axis_cmd, 1.0, kTolerance);
	DcsBridge::route_command(systems, DcsIds::Commands::ThrottleDecrease, 1.0f);
	TEST_EXPECT_NEAR(context, systems.throttle_inputs.left.keyboard_cmd, 0.9925, kTolerance);

	DcsBridge::route_command(systems, DcsIds::Commands::AirBrakesOn, 1.0f);
	DcsBridge::route_command(systems, DcsIds::Commands::FlapsDown, 1.0f);
	DcsBridge::route_command(systems, DcsIds::Commands::GearDown, 1.0f);
	TEST_EXPECT(context, systems.airframe_devices.airbrake_switch);
	TEST_EXPECT(context, systems.airframe_devices.flap_mode == Systems::FLAP_MODE_DOWN);
	TEST_EXPECT(context, systems.landing_gear.switch_down);
}

void test_wheel_commands(Tests::Context& context)
{
	Core::Fck1cEfmSystems systems;
	DcsBridge::route_command(systems, DcsIds::Commands::NoseTurnDown, 1.0f);
	DcsBridge::route_command(systems, DcsIds::Commands::WheelBrakeLeftOn, 1.0f);
	DcsBridge::route_command(systems, DcsIds::Commands::WheelBrakeRightOn, 1.0f);
	TEST_EXPECT(context, systems.landing_gear.wheels.nose_turn_enabled);
	TEST_EXPECT_NEAR(context, systems.landing_gear.wheels.brake_left, 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, systems.landing_gear.wheels.brake_right, 1.0, kTolerance);

	DcsBridge::route_command(systems, DcsIds::Commands::WheelBrakeOff, 1.0f);
	TEST_EXPECT_NEAR(context, systems.landing_gear.wheels.brake, 0.0, kTolerance);
}
}

void run_dcs_command_router_tests(Tests::Context& context)
{
	test_primary_controls(context);
	test_fbw_and_engine_commands(context);
	test_throttle_and_airframe_commands(context);
	test_wheel_commands(context);
}
