#include "SystemPipelineTestFixture.h"

#include <stdexcept>

namespace
{
using namespace Core;
using namespace Core::Systems;
using namespace SystemPipelineTest;

constexpr double kCommandValue = 0.75;
constexpr double kNeutralValue = 0.0;
constexpr double kDamagedIntegrity = 0.5;
constexpr double kTolerance = 1e-12;
constexpr std::size_t kFirstDamageSegment = 0;
constexpr std::size_t kRepairSubscriberCount = 2;
constexpr int kNoHandlerCalls = 0;
constexpr int kOneHandlerCall = 1;

SystemDefinition handler_system(
	const std::string& id,
	const SetupAction& setup)
{
	return { id, SystemGroup::Control, setup, no_step() };
}

FuelManagementHandlers complete_fuel_management()
{
	return {
		[]() { return FlightFuelState{}; },
		[]() { return FuelData{}; },
		[](double) {},
		[](const ExternalFuelInput&) {},
		[]() {}
	};
}

void test_incomplete_fuel_management_fails(Tests::Context& context)
{
	const SystemDefinition system = {
		"fuel",
		SystemGroup::Equipment,
		[](SystemSetup& setup)
		{
			setup.publish(AircraftDataKeys::kFuelData, FuelData{});
			setup.register_fuel_management({});
		},
		no_step()
	};
	TEST_EXPECT(context, construction_throws({ entry(system) }));
}

void test_fuel_management_requires_fuel_data_owner(
	Tests::Context& context)
{
	const SystemDefinition manager = handler_system(
		"manager",
		[](SystemSetup& setup)
		{
			setup.register_fuel_management(complete_fuel_management());
		});
	const SystemDefinition publisher = {
		"publisher",
		SystemGroup::Equipment,
		[](SystemSetup& setup)
		{
			setup.publish(AircraftDataKeys::kFuelData, FuelData{});
		},
		no_step()
	};
	TEST_EXPECT(
		context,
		construction_throws({ entry(manager), entry(publisher) }));
}

void test_duplicate_fuel_management_registration_fails(
	Tests::Context& context)
{
	const SystemDefinition system = {
		"fuel",
		SystemGroup::Equipment,
		[](SystemSetup& setup)
		{
			setup.publish(AircraftDataKeys::kFuelData, FuelData{});
			setup.register_fuel_management(complete_fuel_management());
			setup.register_fuel_management(complete_fuel_management());
		},
		no_step()
	};
	TEST_EXPECT(context, construction_throws({ entry(system) }));
}

void test_duplicate_command_handler_fails(Tests::Context& context)
{
	const SetupAction register_pitch = [](SystemSetup& setup)
	{
		setup.register_command_handler(
			CommandId::SetPitchAxis,
			[](const Command&) {});
	};
	TEST_EXPECT(
		context,
		construction_throws({
			entry(handler_system("first", register_pitch)),
			entry(handler_system("second", register_pitch))
		}));
}

void test_unregistered_command_is_explicitly_ignored(Tests::Context& context)
{
	SystemPipeline pipeline(flight_setup(), {});
	const DispatchResult result = pipeline.send({
		CommandGroup::PitchRoll,
		CommandId::SetPitchAxis,
		kCommandValue
	});
	TEST_EXPECT(context, result == DispatchResult::Unhandled);
}

void test_command_only_changes_next_step_request(Tests::Context& context)
{
	auto requested = std::make_shared<double>(kNeutralValue);
	const SystemDefinition system = {
		"controlled",
		SystemGroup::Control,
		[requested](SystemSetup& setup)
		{
			setup.publish(
				AircraftDataKeys::kFlightControlDemand,
				demand(kNeutralValue));
			setup.register_command_handler(
				CommandId::SetPitchAxis,
				[requested](const Command& command)
				{
					*requested = command.value;
				});
		},
		[requested](const AircraftDataSnapshot&, SystemResult& result)
		{
			result.publish(
				AircraftDataKeys::kFlightControlDemand,
				demand(*requested));
		}
	};
	SystemPipeline pipeline(flight_setup(), { entry(system) });
	TEST_EXPECT(
		context,
		pipeline.send({
			CommandGroup::PitchRoll,
			CommandId::SetPitchAxis,
			kCommandValue
		}) == DispatchResult::Handled);
	TEST_EXPECT_NEAR(
		context,
		pipeline.snapshot().read(
			AircraftDataKeys::kFlightControlDemand).pitch,
		kNeutralValue,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		step_pipeline(pipeline).read(
			AircraftDataKeys::kFlightControlDemand).pitch,
		kCommandValue,
		kTolerance);
}

void test_duplicate_damage_owner_fails(Tests::Context& context)
{
	const SetupAction register_left_wing = [](SystemSetup& setup)
	{
		setup.register_damage_handler(
			DamageArea::LeftWing,
			[](const DamageEvent&) {});
	};
	TEST_EXPECT(
		context,
		construction_throws({
			entry(handler_system("first", register_left_wing)),
			entry(handler_system("second", register_left_wing))
		}));
}

void test_damage_routes_to_semantic_owner(Tests::Context& context)
{
	auto left_hits = std::make_shared<int>(kNoHandlerCalls);
	auto right_hits = std::make_shared<int>(kNoHandlerCalls);
	const SystemDefinition left = handler_system(
		"left",
		[left_hits](SystemSetup& setup)
		{
			setup.register_damage_handler(
				DamageArea::LeftWing,
				[left_hits](const DamageEvent&) { ++*left_hits; });
		});
	const SystemDefinition right = handler_system(
		"right",
		[right_hits](SystemSetup& setup)
		{
			setup.register_damage_handler(
				DamageArea::RightWing,
				[right_hits](const DamageEvent&) { ++*right_hits; });
		});
	SystemPipeline pipeline(flight_setup(), { entry(right), entry(left) });
	TEST_EXPECT(
		context,
		pipeline.apply({
			DamageArea::LeftWing,
			kFirstDamageSegment,
			kDamagedIntegrity
		}) ==
			DispatchResult::Handled);
	TEST_EXPECT(context, *left_hits == kOneHandlerCall);
	TEST_EXPECT(context, *right_hits == kNoHandlerCalls);
	TEST_EXPECT(
		context,
		pipeline.apply({
			DamageArea::Tail,
			kFirstDamageSegment,
			kDamagedIntegrity
		}) ==
			DispatchResult::Unhandled);
}

void test_repair_reaches_every_subscriber(Tests::Context& context)
{
	auto first_repairs = std::make_shared<int>(kNoHandlerCalls);
	auto second_repairs = std::make_shared<int>(kNoHandlerCalls);
	const SystemDefinition first = handler_system(
		"first",
		[first_repairs](SystemSetup& setup)
		{
			setup.register_repair_handler(
				[first_repairs](const RepairEvent&) { ++*first_repairs; });
		});
	const SystemDefinition second = handler_system(
		"second",
		[second_repairs](SystemSetup& setup)
		{
			setup.register_repair_handler(
				[second_repairs](const RepairEvent&) { ++*second_repairs; });
		});
	SystemPipeline pipeline(flight_setup(), { entry(first), entry(second) });
	TEST_EXPECT(
		context,
		pipeline.apply(RepairEvent{}) == kRepairSubscriberCount);
	TEST_EXPECT(context, *first_repairs == kOneHandlerCall);
	TEST_EXPECT(context, *second_repairs == kOneHandlerCall);
}

void test_handler_error_does_not_publish_frame(Tests::Context& context)
{
	const SystemDefinition system = {
		"throwing",
		SystemGroup::Control,
		[](SystemSetup& setup)
		{
			setup.publish(
				AircraftDataKeys::kFlightControlDemand,
				demand(kNeutralValue));
			setup.register_command_handler(
				CommandId::SetPitchAxis,
				[](const Command&)
				{
					throw std::runtime_error("expected handler failure");
				});
		},
		no_step()
	};
	SystemPipeline pipeline(flight_setup(), { entry(system) });
	TEST_EXPECT(
		context,
		action_throws([&pipeline]()
		{
			(void)pipeline.send({
				CommandGroup::PitchRoll,
				CommandId::SetPitchAxis,
				kCommandValue
			});
		}));
	TEST_EXPECT_NEAR(
		context,
		pipeline.snapshot().read(
			AircraftDataKeys::kFlightControlDemand).pitch,
		kNeutralValue,
		kTolerance);
}
}

void run_system_pipeline_handler_tests(Tests::Context& context)
{
	test_incomplete_fuel_management_fails(context);
	test_fuel_management_requires_fuel_data_owner(context);
	test_duplicate_fuel_management_registration_fails(context);
	test_duplicate_command_handler_fails(context);
	test_unregistered_command_is_explicitly_ignored(context);
	test_command_only_changes_next_step_request(context);
	test_duplicate_damage_owner_fails(context);
	test_damage_routes_to_semantic_owner(context);
	test_repair_reaches_every_subscriber(context);
	test_handler_error_does_not_publish_frame(context);
}
