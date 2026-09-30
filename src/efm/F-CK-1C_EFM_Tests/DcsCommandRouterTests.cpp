#include "TestHarness.h"

#include "DcsBridge/Internal/DcsCommandRouter.h"
#include "DcsIds/Commands.h"

#include <iterator>
#include <limits>

namespace
{
constexpr double kTolerance = 1e-6;
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
	EXPECT_COMMAND(WheelBrakeRightOff, SetRightBrake),
	EXPECT_COMMAND(TriggerFirstStage, SetTriggerFirstStage),
	EXPECT_COMMAND(CMSForward, StartCmsForward),
	EXPECT_COMMAND(CMSAft, StartCmsAft),
	EXPECT_COMMAND(CMSLeft, StartCmsLeft),
	EXPECT_COMMAND(CMSRight, StartCmsRight),
	EXPECT_COMMAND(CMSPress, PressCms),
	EXPECT_COMMAND(TriggerSecondStage, SetTriggerSecondStage),
	EXPECT_COMMAND(MasterArmOn, SetMasterArmOn),
	EXPECT_COMMAND(MasterArmOff, SetMasterArmOff),
	EXPECT_COMMAND(MasterArmSim, SetMasterArmSim),
	EXPECT_COMMAND(DogfightSwitch, SelectDogfightMode),
	EXPECT_COMMAND(MissileUncage, SetMissileUncage),
	EXPECT_COMMAND(WeaponRelease, SetWeaponRelease),
	EXPECT_COMMAND(TMSUp, SetTmsUp),
	EXPECT_COMMAND(TMSDown, PressTmsDown),
	EXPECT_COMMAND(TMSLeft, PressTmsLeft),
	EXPECT_COMMAND(TMSRight, PressTmsRight),
	EXPECT_COMMAND(NavMode, SelectNavigationMode),
	EXPECT_COMMAND(MissileOverride, SelectMissileOverride),
	EXPECT_COMMAND(EngineThrustCutTestToggle, ToggleThrustCutTest),
	EXPECT_COMMAND(EngineThrustCutTestEnable, EnableThrustCutTest),
	EXPECT_COMMAND(EngineThrustCutTestDisable, DisableThrustCutTest)
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
	TEST_EXPECT_NEAR(
		context,
		mapping.command.value_normalized,
		expected.value_normalized,
		kTolerance);
}

void test_system_command_mappings(Tests::Context& context)
{
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

void test_mapping_rules_and_errors(Tests::Context& context)
{
	const DcsBridge::CommandTableValidation table =
		DcsBridge::validate_command_bindings();
	TEST_EXPECT(context, table.error == DcsBridge::CommandBindingError::None);
	const DcsBridge::DcsCommandMapping press =
		DcsBridge::map_command(DcsIds::Commands::NoseTurnToggle, 1.0F);
	TEST_EXPECT(context, press.should_dispatch());
	TEST_EXPECT_NEAR(context, press.command.value_normalized, 1.0, kTolerance);
	const DcsBridge::DcsCommandMapping release =
		DcsBridge::map_command(DcsIds::Commands::NoseTurnToggle, 0.0F);
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

void test_non_flight_control_commands_have_expected_semantics(
	Tests::Context& context)
{
	for (const ExpectedSemanticCommand& expected : kExpectedSemanticCommands)
	{
		const DcsBridge::DcsCommandMapping mapping =
			DcsBridge::inspect_command_binding(
				expected.dcs_id,
				kMappingProbeValue);
		TEST_EXPECT(context, mapping.should_dispatch());
		TEST_EXPECT(context, mapping.command.id == expected.command_id);
	}
}

void test_inactive_cockpit_binding_value_rules(Tests::Context& context)
{
	const DcsBridge::DcsCommandMapping held_release =
		DcsBridge::inspect_command_binding(
			DcsIds::Commands::MissileUncage,
			0.0F);
	TEST_EXPECT(context, held_release.should_dispatch());
	TEST_EXPECT(
		context,
		held_release.command.id == Core::CommandId::SetMissileUncage);
	TEST_EXPECT_NEAR(
		context, held_release.command.value_normalized, 0.0, kTolerance);
	const DcsBridge::DcsCommandMapping press_release =
		DcsBridge::inspect_command_binding(
			DcsIds::Commands::TMSDown,
			0.0F);
	TEST_EXPECT(
		context,
		press_release.status ==
			DcsBridge::DcsCommandMappingStatus::IgnoredRelease);
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
		const DcsBridge::DcsCommandMapping non_finite =
			DcsBridge::map_command(entry.id, std::numeric_limits<float>::infinity());
		if (entry.route == DcsIds::CommandRouting::Route::Cockpit)
		{
			TEST_EXPECT(context, mapping.status ==
				DcsBridge::DcsCommandMappingStatus::IgnoredCommand);
			TEST_EXPECT(context, non_finite.status ==
				DcsBridge::DcsCommandMappingStatus::IgnoredCommand);
			continue;
		}
		TEST_EXPECT(context, mapping.status ==
			DcsBridge::DcsCommandMappingStatus::UnknownCommand);
		TEST_EXPECT(context, non_finite.status ==
			DcsBridge::DcsCommandMappingStatus::InvalidValue);
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
	test_system_command_mappings(context);
	test_mapping_rules_and_errors(context);
	test_non_flight_control_commands_have_expected_semantics(context);
	test_inactive_cockpit_binding_value_rules(context);
	test_generated_command_routes(context);
	test_generated_ignored_dcs_commands(context);
	test_sensor_command_id_contract(context);
}
