#include "TestHarness.h"

#include "Core/Fck1cEfm.h"
#include "DcsBridge/Internal/DcsCommandRouter.h"
#include "DcsIds/Commands.h"

#include <iterator>
#include <limits>

namespace
{
constexpr double kTolerance = 1e-6;
constexpr double kSimulationStepS = 0.001;
constexpr float kMappingProbeValue = 1.0F;
constexpr int kUnknownCommandId = 2659;
constexpr int kDcsRadarOnOffCommandId = 86;
constexpr int kDcsEosOnOffCommandId = 87;

struct DcsCommandInput
{
	int command = 0;
	float value = 0.0F;
};

struct ExpectedSemanticCommand
{
	int dcs_id = 0;
	Core::CommandId command_id = Core::CommandId::NoOp;
};

#define EXPECT_COMMAND(id, command_id) \
	{ DcsIds::Commands::id, Core::CommandId::command_id }

constexpr ExpectedSemanticCommand kExpectedSemanticCommands[] = {
	EXPECT_COMMAND(JoystickPitch, SetPitchAxis),
	EXPECT_COMMAND(PitchUp, SetPitchDiscrete),
	EXPECT_COMMAND(PitchUpStop, SetPitchDiscrete),
	EXPECT_COMMAND(PitchDown, SetPitchDiscrete),
	EXPECT_COMMAND(PitchDownStop, SetPitchDiscrete),
	EXPECT_COMMAND(TrimUp, AdjustPitchTrim),
	EXPECT_COMMAND(TrimDown, AdjustPitchTrim),
	EXPECT_COMMAND(JoystickRoll, SetRollAxis),
	EXPECT_COMMAND(RollLeft, SetRollDiscrete),
	EXPECT_COMMAND(RollLeftStop, SetRollDiscrete),
	EXPECT_COMMAND(RollRight, SetRollDiscrete),
	EXPECT_COMMAND(RollRightStop, SetRollDiscrete),
	EXPECT_COMMAND(TrimLeft, AdjustRollTrim),
	EXPECT_COMMAND(TrimRight, AdjustRollTrim),
	EXPECT_COMMAND(PedalYaw, SetYawAxis),
	EXPECT_COMMAND(RudderLeft, SetYawDiscrete),
	EXPECT_COMMAND(RudderLeftStop, SetYawDiscrete),
	EXPECT_COMMAND(RudderRight, SetYawDiscrete),
	EXPECT_COMMAND(RudderRightStop, SetYawDiscrete),
	EXPECT_COMMAND(RudderTrimLeft, AdjustYawTrim),
	EXPECT_COMMAND(RudderTrimRight, AdjustYawTrim),
	EXPECT_COMMAND(ResetTrim, ResetTrim),
	EXPECT_COMMAND(FBWCatToggle, ToggleFbwCat),
	EXPECT_COMMAND(FBWCat1, SetFbwCat1),
	EXPECT_COMMAND(FBWCat3, SetFbwCat3),
	EXPECT_COMMAND(FBWGLimiterOverride, SetGLimiterOverride),
	EXPECT_COMMAND(FBWGLimiterOverrideToggle, ToggleGLimiterOverride),
	EXPECT_COMMAND(EnginesOn, SetBothEngines),
	EXPECT_COMMAND(LeftEngineOn, SetLeftEngine),
	EXPECT_COMMAND(RightEngineOn, SetRightEngine),
	EXPECT_COMMAND(EnginesOff, SetBothEngines),
	EXPECT_COMMAND(LeftEngineOff, SetLeftEngine),
	EXPECT_COMMAND(RightEngineOff, SetRightEngine),
	EXPECT_COMMAND(ThrottleAxis, SetCommonThrottleAxis),
	EXPECT_COMMAND(ThrottleAxisLeft, SetLeftThrottleAxis),
	EXPECT_COMMAND(ThrottleAxisRight, SetRightThrottleAxis),
	EXPECT_COMMAND(ThrottleIncrease, StepCommonThrottle),
	EXPECT_COMMAND(ThrottleLeftUp, StepLeftThrottle),
	EXPECT_COMMAND(ThrottleRightUp, StepRightThrottle),
	EXPECT_COMMAND(ThrottleDecrease, StepCommonThrottle),
	EXPECT_COMMAND(ThrottleLeftDown, StepLeftThrottle),
	EXPECT_COMMAND(ThrottleRightDown, StepRightThrottle),
	EXPECT_COMMAND(ThrottleStop, NoOp),
	EXPECT_COMMAND(AirBrakes, ToggleAirbrake),
	EXPECT_COMMAND(AirBrakesOff, SetAirbrake),
	EXPECT_COMMAND(AirBrakesOn, SetAirbrake),
	EXPECT_COMMAND(AirBrakesAuto, NoOp),
	EXPECT_COMMAND(AirBrakesUp, SetAirbrake),
	EXPECT_COMMAND(AirBrakesDown, SetAirbrake),
	EXPECT_COMMAND(FlapsToggle, ToggleFlaps),
	EXPECT_COMMAND(FlapsDown, SetFlapsDown),
	EXPECT_COMMAND(FlapsUp, SetFlapsUp),
	EXPECT_COMMAND(FlapsAuto, SetFlapsAuto),
	EXPECT_COMMAND(FlapsUpCmd, SetFlapsUp),
	EXPECT_COMMAND(FlapsDownCmd, SetFlapsDown),
	EXPECT_COMMAND(GearToggle, ToggleGear),
	EXPECT_COMMAND(GearDown, SetGear),
	EXPECT_COMMAND(GearUp, SetGear),
	EXPECT_COMMAND(GearAuto, NoOp),
	EXPECT_COMMAND(GearHandleUp, SetGear),
	EXPECT_COMMAND(GearHandleDown, SetGear),
	EXPECT_COMMAND(NoseTurnToggle, ToggleNoseWheelSteering),
	EXPECT_COMMAND(NoseTurnUp, SetNoseWheelSteering),
	EXPECT_COMMAND(NoseTurnAuto, SetNoseWheelSteering),
	EXPECT_COMMAND(NoseTurnDown, SetNoseWheelSteering),
	EXPECT_COMMAND(WheelBrakeAxis, SetBrake),
	EXPECT_COMMAND(WheelBrakeAxisLeft, SetLeftBrake),
	EXPECT_COMMAND(WheelBrakeAxisRight, SetRightBrake),
	EXPECT_COMMAND(WheelBrakeOn, SetBrake),
	EXPECT_COMMAND(WheelBrakeOff, SetBrake),
	EXPECT_COMMAND(WheelBrakeLeftOn, SetLeftBrake),
	EXPECT_COMMAND(WheelBrakeLeftOff, SetLeftBrake),
	EXPECT_COMMAND(WheelBrakeRightOn, SetRightBrake),
	EXPECT_COMMAND(WheelBrakeRightOff, SetRightBrake)
};

#undef EXPECT_COMMAND

void expect_mapping(
	Tests::Context& context,
	const DcsCommandInput& input,
	const Core::Command& expected)
{
	const DcsBridge::DcsCommandMapping mapping =
		DcsBridge::map_command(input.command, input.value);
	TEST_EXPECT(context, mapping.should_dispatch());
	TEST_EXPECT(context, mapping.command.id == expected.id);
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
		{ Core::CommandId::SetPitchAxis, 0.4 });
	expect_mapping(
		context,
		{ DcsIds::Commands::JoystickRoll, -0.3F },
		{ Core::CommandId::SetRollAxis, -0.3 });
	expect_mapping(
		context,
		{ DcsIds::Commands::PedalYaw, 0.2F },
		{ Core::CommandId::SetYawAxis, 0.2 });
	expect_mapping(
		context,
		{ DcsIds::Commands::TrimUp, 1.0F },
		{ Core::CommandId::AdjustPitchTrim, 0.0015 });
}

void test_system_command_mappings(Tests::Context& context)
{
	expect_mapping(
		context,
		{ DcsIds::Commands::FBWCat3, 1.0F },
		{ Core::CommandId::SetFbwCat3, 1.0 });
	expect_mapping(
		context,
		{ DcsIds::Commands::LeftEngineOff, 1.0F },
		{ Core::CommandId::SetLeftEngine, 0.0 });
	expect_mapping(
		context,
		{ DcsIds::Commands::ThrottleAxis, -1.0F },
		{ Core::CommandId::SetCommonThrottleAxis, -1.0 });
	expect_mapping(
		context,
		{ DcsIds::Commands::WheelBrakeLeftOn, 1.0F },
		{ Core::CommandId::SetLeftBrake, 1.0 });
}

void test_routed_primary_and_engine_outputs(Tests::Context& context)
{
	Core::Fck1cEfm efm;
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
	Core::Fck1cEfm baseline;
	Core::Fck1cEfm trimmed;
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
	Core::Fck1cEfm efm;
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
	Core::Fck1cEfm efm;
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

void test_all_raw_commands_have_expected_semantics(Tests::Context& context)
{
	const DcsBridge::CommandTableValidation table =
		DcsBridge::validate_command_bindings();
	TEST_EXPECT(context, table.binding_count == std::size(kExpectedSemanticCommands));
	for (const ExpectedSemanticCommand& expected : kExpectedSemanticCommands)
	{
		const DcsBridge::DcsCommandMapping mapping =
			DcsBridge::map_command(expected.dcs_id, kMappingProbeValue);
		TEST_EXPECT(context, mapping.should_dispatch());
		TEST_EXPECT(context, mapping.command.id == expected.command_id);
	}
}

void test_generated_command_routes(Tests::Context& context)
{
	for (const DcsIds::CommandRouting::Entry& entry :
		DcsIds::CommandRouting::CustomCommands)
	{
		const DcsBridge::DcsCommandMapping mapping =
			DcsBridge::map_command(entry.id, 1.0F);
		if (entry.route == DcsIds::CommandRouting::Route::Efm)
		{
			TEST_EXPECT(context, mapping.should_dispatch());
			continue;
		}
		TEST_EXPECT(context,
			mapping.status == DcsBridge::DcsCommandMappingStatus::IgnoredCommand);
		const DcsBridge::DcsCommandMapping non_finite =
			DcsBridge::map_command(entry.id, std::numeric_limits<float>::infinity());
		TEST_EXPECT(context,
			non_finite.status == DcsBridge::DcsCommandMappingStatus::IgnoredCommand);
	}
}

void test_generated_ignored_dcs_commands(Tests::Context& context)
{
	for (const int command : DcsIds::CommandRouting::IgnoredDcsCommands)
	{
		const DcsBridge::DcsCommandMapping mapping =
			DcsBridge::map_command(command, 1.0F);
		TEST_EXPECT(context,
			mapping.status == DcsBridge::DcsCommandMappingStatus::IgnoredCommand);
		const DcsBridge::DcsCommandMapping non_finite =
			DcsBridge::map_command(command, std::numeric_limits<float>::infinity());
		TEST_EXPECT(context,
			non_finite.status == DcsBridge::DcsCommandMappingStatus::IgnoredCommand);
	}
}

void test_sensor_command_id_contract(Tests::Context& context)
{
	TEST_EXPECT(context,
		DcsIds::DcsCommands::iCommandPlaneRadarOnOff ==
			kDcsRadarOnOffCommandId);
	TEST_EXPECT(context,
		DcsIds::DcsCommands::iCommandPlaneEOSOnOff ==
			kDcsEosOnOffCommandId);
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
	test_all_raw_commands_have_expected_semantics(context);
	test_generated_command_routes(context);
	test_generated_ignored_dcs_commands(context);
	test_sensor_command_id_contract(context);
}
