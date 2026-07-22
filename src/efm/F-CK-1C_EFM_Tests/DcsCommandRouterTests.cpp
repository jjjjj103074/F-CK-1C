#include "TestHarness.h"

#include "Core/Fck1cEfm.h"
#include "Data/AircraftConfig.h"
#include "DcsBridge/DcsCommandRouter.h"
#include "DcsIds/Commands.h"

#include <limits>

namespace
{
constexpr double kTolerance = 1e-6;
constexpr double kSimulationStepS = 0.001;
constexpr int kUnknownCommandId = -1;

struct DcsCommandInput
{
	int command = 0;
	float value = 0.0F;
};

void expect_mapping(
	Tests::Context& context,
	const DcsCommandInput& input,
	const Core::EfmCommand& expected)
{
	const DcsBridge::DcsCommandMapping mapping =
		DcsBridge::map_command(input.command, input.value);
	TEST_EXPECT(context, mapping.should_dispatch());
	TEST_EXPECT(context, mapping.command.group == expected.group);
	TEST_EXPECT(context, mapping.command.action == expected.action);
	TEST_EXPECT_NEAR(context, mapping.command.value, expected.value, kTolerance);
}

void route_command(
	Tests::Context& context,
	Core::Fck1cEfm& efm,
	const DcsCommandInput& input)
{
	const DcsBridge::DcsCommandMapping mapping =
		DcsBridge::map_command(input.command, input.value);
	TEST_EXPECT(context, mapping.should_dispatch());
	if (mapping.should_dispatch())
	{
		efm.handle_command(mapping.command);
	}
}

Core::FrameOutput step(Core::Fck1cEfm& efm)
{
	Core::FrameInput input;
	input.dt_s = kSimulationStepS;
	input.max_power = { 1.0, 1.0 };
	return efm.step(input);
}

void test_primary_control_mappings(Tests::Context& context)
{
	expect_mapping(
		context,
		{ DcsIds::Commands::JoystickPitch, 0.4F },
		{ Core::CommandGroup::PitchRoll, Core::CommandAction::SetPitchAxis, 0.4 });
	expect_mapping(
		context,
		{ DcsIds::Commands::JoystickRoll, -0.3F },
		{ Core::CommandGroup::PitchRoll, Core::CommandAction::SetRollAxis, -0.3 });
	expect_mapping(
		context,
		{ DcsIds::Commands::PedalYaw, 0.2F },
		{ Core::CommandGroup::Yaw, Core::CommandAction::SetYawAxis, 0.2 });
	expect_mapping(
		context,
		{ DcsIds::Commands::TrimUp, 1.0F },
		{ Core::CommandGroup::PitchRoll, Core::CommandAction::AdjustPitchTrim, 0.0015 });
}

void test_system_command_mappings(Tests::Context& context)
{
	expect_mapping(
		context,
		{ DcsIds::Commands::FBWCat3, 1.0F },
		{ Core::CommandGroup::Fbw, Core::CommandAction::SetFbwCat3, 1.0 });
	expect_mapping(
		context,
		{ DcsIds::Commands::LeftEngineOff, 1.0F },
		{ Core::CommandGroup::Engine, Core::CommandAction::SetLeftEngine, 0.0 });
	expect_mapping(
		context,
		{ DcsIds::Commands::ThrottleAxis, -1.0F },
		{ Core::CommandGroup::Throttle, Core::CommandAction::SetCommonThrottleAxis, -1.0 });
	expect_mapping(
		context,
		{ DcsIds::Commands::WheelBrakeLeftOn, 1.0F },
		{ Core::CommandGroup::LandingGear, Core::CommandAction::SetLeftBrake, 1.0 });
}

void test_routed_primary_and_engine_outputs(Tests::Context& context)
{
	Core::Fck1cEfm efm(Data::fck1c_aircraft_config());
	(void)efm.start(Core::StartMode::HotGround);
	efm.set_internal_fuel(100.0);
	route_command(context, efm, { DcsIds::Commands::JoystickPitch, 0.4F });
	route_command(context, efm, { DcsIds::Commands::JoystickRoll, -0.3F });
	route_command(context, efm, { DcsIds::Commands::PedalYaw, 0.2F });
	route_command(context, efm, { DcsIds::Commands::EnginesOn, 1.0F });
	route_command(context, efm, { DcsIds::Commands::LeftEngineOff, 1.0F });
	const Core::FrameOutput output = step(efm);
	TEST_EXPECT_NEAR(context, output.controls.pitch_input, 0.4, kTolerance);
	TEST_EXPECT_NEAR(context, output.controls.roll_input, -0.3, kTolerance);
	TEST_EXPECT_NEAR(context, output.controls.yaw_input, -0.2, kTolerance);
	TEST_EXPECT(context, !output.engines[0].switch_on);
	TEST_EXPECT(context, output.engines[1].switch_on);
}

void test_routed_trim_changes_control_output(Tests::Context& context)
{
	Core::Fck1cEfm baseline(Data::fck1c_aircraft_config());
	Core::Fck1cEfm trimmed(Data::fck1c_aircraft_config());
	(void)baseline.start(Core::StartMode::HotGround);
	(void)trimmed.start(Core::StartMode::HotGround);
	route_command(context, trimmed, { DcsIds::Commands::TrimUp, 1.0F });
	const Core::FrameOutput baseline_output = step(baseline);
	const Core::FrameOutput trimmed_output = step(trimmed);
	TEST_EXPECT(
		context,
		trimmed_output.controls.elevator_command !=
			baseline_output.controls.elevator_command);
}

void test_routed_throttle_and_airframe_outputs(Tests::Context& context)
{
	Core::Fck1cEfm efm(Data::fck1c_aircraft_config());
	(void)efm.start(Core::StartMode::HotAir);
	route_command(context, efm, { DcsIds::Commands::ThrottleAxis, -1.0F });
	route_command(context, efm, { DcsIds::Commands::AirBrakesOn, 1.0F });
	route_command(context, efm, { DcsIds::Commands::FlapsDown, 1.0F });
	route_command(context, efm, { DcsIds::Commands::GearDown, 1.0F });
	const Core::FrameOutput output = step(efm);
	TEST_EXPECT_NEAR(context, output.engines[0].throttle_input, 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, output.engines[1].throttle_input, 1.0, kTolerance);
	TEST_EXPECT(context, output.controls.airbrake_position > 0.0);
	TEST_EXPECT(context, output.controls.flaps_position > 0.0);
	TEST_EXPECT(context, output.landing_gear.gear_position > 0.0);
}

void test_routed_wheel_outputs(Tests::Context& context)
{
	Core::Fck1cEfm efm(Data::fck1c_aircraft_config());
	(void)efm.start(Core::StartMode::HotGround);
	route_command(context, efm, { DcsIds::Commands::NoseTurnUp, 1.0F });
	route_command(context, efm, { DcsIds::Commands::PedalYaw, 0.5F });
	TEST_EXPECT_NEAR(context, step(efm).landing_gear.nose_wheel_steering,
		0.0, kTolerance);
	route_command(context, efm, { DcsIds::Commands::NoseTurnDown, 1.0F });
	route_command(context, efm, { DcsIds::Commands::WheelBrakeLeftOn, 1.0F });
	route_command(context, efm, { DcsIds::Commands::WheelBrakeRightOn, 1.0F });
	const Core::FrameOutput output = step(efm);
	TEST_EXPECT(context, output.landing_gear.nose_wheel_steering > 0.0);
	TEST_EXPECT_NEAR(context, output.landing_gear.brake_left, 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, output.landing_gear.brake_right, 1.0, kTolerance);
}

void test_mapping_rules_and_errors(Tests::Context& context)
{
	const DcsBridge::CommandTableValidation table =
		DcsBridge::validate_command_bindings();
	TEST_EXPECT(context, table.error == DcsBridge::CommandBindingError::None);
	const DcsBridge::DcsCommandMapping press =
		DcsBridge::map_command(DcsIds::Commands::FBWCatToggle, 0.1F);
	TEST_EXPECT(context, press.should_dispatch());
	TEST_EXPECT_NEAR(context, press.command.value, 1.0, kTolerance);
	const DcsBridge::DcsCommandMapping release =
		DcsBridge::map_command(DcsIds::Commands::FBWCatToggle, 0.0F);
	TEST_EXPECT(
		context,
		release.status == DcsBridge::DcsCommandMappingStatus::IgnoredRelease);
	const DcsBridge::DcsCommandMapping unknown =
		DcsBridge::map_command(kUnknownCommandId, 0.5F);
	TEST_EXPECT(
		context,
		unknown.status == DcsBridge::DcsCommandMappingStatus::UnknownCommand);
	const DcsBridge::DcsCommandMapping invalid = DcsBridge::map_command(
		DcsIds::Commands::JoystickPitch,
		std::numeric_limits<float>::infinity());
	TEST_EXPECT(
		context,
		invalid.status == DcsBridge::DcsCommandMappingStatus::InvalidValue);
}
}

void run_dcs_command_router_tests(Tests::Context& context)
{
	test_primary_control_mappings(context);
	test_system_command_mappings(context);
	test_routed_primary_and_engine_outputs(context);
	test_routed_trim_changes_control_output(context);
	test_routed_throttle_and_airframe_outputs(context);
	test_routed_wheel_outputs(context);
	test_mapping_rules_and_errors(context);
}
