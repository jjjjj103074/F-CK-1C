#include "SystemPipelineTestFixture.h"

#include <algorithm>
#include <stdexcept>

namespace
{
using namespace Core;
using namespace Core::Systems;
using namespace SystemPipelineTest;

constexpr double kInitialDemand = 1.0;
constexpr double kNextDemand = 2.0;
constexpr double kPendingDemand = 5.0;
constexpr double kNeutralValue = 0.0;
constexpr double kInitialCrossPosition = 2.0;
constexpr double kDemandOffset = 10.0;
constexpr double kPositionOffset = 20.0;
constexpr double kTolerance = 1e-12;
constexpr double kFullIntegrity = 1.0;
constexpr int kFirstCall = 0;
constexpr std::size_t kPhaseThreeCatalogSize = 7;

struct DemandPublisherOptions
{
	std::string id;
	SystemGroup group;
	double initial;
	double next;
};

SystemDefinition demand_publisher(const DemandPublisherOptions& options)
{
	return {
		options.id,
		options.group,
		[options](SystemSetup& setup)
		{
			setup.publish(
				AircraftDataKeys::kFlightControlDemand,
				demand(options.initial));
		},
		[options](const AircraftDataSnapshot&, SystemResult& result)
		{
			result.publish(
				AircraftDataKeys::kFlightControlDemand,
				demand(options.next));
		}
	};
}

SystemDefinition demand_observer(SystemGroup group)
{
	return {
		"observer",
		group,
		[](SystemSetup& setup)
		{
			setup.read(AircraftDataKeys::kFlightControlDemand);
			setup.publish(
				AircraftDataKeys::kPrimaryControlPosition,
				position(kNeutralValue));
		},
		[](const AircraftDataSnapshot& snapshot, SystemResult& result)
		{
			const double pitch =
				snapshot.read(AircraftDataKeys::kFlightControlDemand).pitch;
			result.publish(
				AircraftDataKeys::kPrimaryControlPosition, position(pitch));
		}
	};
}

void test_missing_provider_fails(Tests::Context& context)
{
	const SystemDefinition reader = {
		"reader",
		SystemGroup::Equipment,
		[](SystemSetup& setup)
		{
			setup.read(AircraftDataKeys::kFlightControlDemand);
		},
		no_step()
	};
	TEST_EXPECT(context, construction_throws({ entry(reader) }));
}

void test_type_mismatch_fails(Tests::Context& context)
{
	const AircraftDataKey<PrimaryControlPosition> wrong_key = {
		AircraftDataId::FlightControlDemand,
		"wrong_demand_type"
	};
	const SystemDefinition publisher = {
		"wrong_type",
		SystemGroup::Control,
		[wrong_key](SystemSetup& setup)
		{
			setup.publish(wrong_key, position(kNeutralValue));
		},
		no_step()
	};
	TEST_EXPECT(context, construction_throws({ entry(publisher) }));
}

void test_required_initial_value_fails(Tests::Context& context)
{
	const SystemDefinition publisher = {
		"publisher",
		SystemGroup::Control,
		[](SystemSetup& setup)
		{
			setup.publish(AircraftDataKeys::kFlightControlDemand);
		},
		no_step()
	};
	const SystemDefinition reader = {
		"reader",
		SystemGroup::Equipment,
		[](SystemSetup& setup)
		{
			setup.read(AircraftDataKeys::kFlightControlDemand);
		},
		no_step()
	};
	TEST_EXPECT(
		context, construction_throws({ entry(publisher), entry(reader) }));
	TEST_EXPECT(
		context, construction_throws({ entry(reader), entry(publisher) }));
}

void test_optional_initial_value_is_explicit(Tests::Context& context)
{
	const SystemDefinition publisher = {
		"publisher",
		SystemGroup::Control,
		[](SystemSetup& setup)
		{
			setup.publish(AircraftDataKeys::kFlightControlDemand);
		},
		no_step()
	};
	const SystemDefinition reader = {
		"reader",
		SystemGroup::Equipment,
		[](SystemSetup& setup)
		{
			setup.read(
				AircraftDataKeys::kFlightControlDemand,
				InitialValueRequirement::Optional);
		},
		no_step()
	};
	SystemPipeline pipeline(
		flight_setup(), { entry(reader), entry(publisher) });
	TEST_EXPECT(
		context,
		!pipeline.snapshot().has(AircraftDataKeys::kFlightControlDemand));
}

void test_duplicate_writer_fails(Tests::Context& context)
{
	const SystemDefinition first =
		demand_publisher({
			"first", SystemGroup::Control, kNeutralValue, kInitialDemand });
	const SystemDefinition second =
		demand_publisher({
			"second",
			SystemGroup::Equipment,
			kNeutralValue,
			kInitialCrossPosition
		});
	TEST_EXPECT(context, construction_throws({ entry(first), entry(second) }));
	TEST_EXPECT(context, construction_throws({ entry(second), entry(first) }));
}

void test_same_group_reads_fixed_snapshot(Tests::Context& context)
{
	const SystemDefinition publisher = demand_publisher({
		"publisher", SystemGroup::Control, kInitialDemand, kNextDemand });
	SystemPipeline pipeline(
		flight_setup(),
		{ entry(publisher), entry(demand_observer(SystemGroup::Control)) });
	const AircraftDataSnapshot output = step_pipeline(pipeline);
	TEST_EXPECT_NEAR(
		context,
		output.read(AircraftDataKeys::kFlightControlDemand).pitch,
		kNextDemand,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		output.read(AircraftDataKeys::kPrimaryControlPosition).elevator,
		kInitialDemand,
		kTolerance);
}

void test_equipment_reads_control_commit(Tests::Context& context)
{
	const SystemDefinition publisher = demand_publisher({
		"publisher", SystemGroup::Control, kInitialDemand, kNextDemand });
	SystemPipeline pipeline(
		flight_setup(),
		{ entry(demand_observer(SystemGroup::Equipment)), entry(publisher) });
	const AircraftDataSnapshot output = step_pipeline(pipeline);
	TEST_EXPECT_NEAR(
		context,
		output.read(AircraftDataKeys::kPrimaryControlPosition).elevator,
		kNextDemand,
		kTolerance);
}

void test_failed_equipment_keeps_previous_frame(Tests::Context& context)
{
	auto observed = std::make_shared<double>(kNeutralValue);
	const SystemDefinition publisher = demand_publisher({
		"publisher", SystemGroup::Control, kInitialDemand, kNextDemand });
	const SystemDefinition failing_equipment = {
		"failing_equipment",
		SystemGroup::Equipment,
		[](SystemSetup& setup)
		{
			setup.read(AircraftDataKeys::kFlightControlDemand);
			setup.publish(
				AircraftDataKeys::kPrimaryControlPosition,
				position(kNeutralValue));
		},
		[observed](const AircraftDataSnapshot& snapshot, SystemResult&)
		{
			*observed =
				snapshot.read(AircraftDataKeys::kFlightControlDemand).pitch;
			throw std::runtime_error("expected equipment failure");
		}
	};
	SystemPipeline pipeline(
		flight_setup(), { entry(publisher), entry(failing_equipment) });
	FrameInput failed_input;
	failed_input.availability.world_kinematics = true;
	failed_input.world_kinematics.velocity.x = kNextDemand;
	TEST_EXPECT(
		context,
		action_throws([&pipeline, &failed_input]()
		{
			const Core::AircraftObservation observation;
			(void)pipeline.step({ failed_input, observation });
		}));
	const AircraftDataSnapshot unchanged = pipeline.snapshot();
	TEST_EXPECT_NEAR(context, *observed, kNextDemand, kTolerance);
	TEST_EXPECT_NEAR(
		context,
		unchanged.read(AircraftDataKeys::kFlightControlDemand).pitch,
		kInitialDemand,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		unchanged.read(AircraftDataKeys::kPrimaryControlPosition).elevator,
		kNeutralValue,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		unchanged.read(AircraftDataKeys::kAircraftObservation).speed_scalar,
		kNeutralValue,
		kTolerance);
}

void test_missing_new_value_retains_last_commit(Tests::Context& context)
{
	auto calls = std::make_shared<int>(kFirstCall);
	const SystemDefinition publisher = {
		"publisher",
		SystemGroup::Control,
		[](SystemSetup& setup)
		{
			setup.publish(
				AircraftDataKeys::kFlightControlDemand,
				demand(kInitialDemand));
		},
		[calls](const AircraftDataSnapshot&, SystemResult& result)
		{
			if ((*calls)++ == kFirstCall)
			{
				result.publish(
					AircraftDataKeys::kFlightControlDemand,
					demand(kNextDemand));
			}
		}
	};
	SystemPipeline pipeline(flight_setup(), { entry(publisher) });
	(void)step_pipeline(pipeline);
	const AircraftDataSnapshot second = step_pipeline(pipeline);
	TEST_EXPECT_NEAR(
		context,
		second.read(AircraftDataKeys::kFlightControlDemand).pitch,
		kNextDemand,
		kTolerance);
}

void test_pending_storage_does_not_leak(Tests::Context& context)
{
	auto calls = std::make_shared<int>(kFirstCall);
	const SystemDefinition publisher = {
		"publisher",
		SystemGroup::Control,
		[](SystemSetup& setup)
		{
			setup.publish(
				AircraftDataKeys::kFlightControlDemand,
				demand(kNeutralValue));
		},
		[calls](const AircraftDataSnapshot&, SystemResult& result)
		{
			if ((*calls)++ == kFirstCall)
			{
				result.publish(
					AircraftDataKeys::kFlightControlDemand,
					demand(kPendingDemand));
				throw std::runtime_error("expected test failure");
			}
		}
	};
	SystemPipeline pipeline(flight_setup(), { entry(publisher) });
	TEST_EXPECT(
		context,
		action_throws([&pipeline]() { (void)step_pipeline(pipeline); }));
	const AircraftDataSnapshot next = step_pipeline(pipeline);
	TEST_EXPECT_NEAR(
		context,
		next.read(AircraftDataKeys::kFlightControlDemand).pitch,
		kNeutralValue,
		kTolerance);
}

SystemDefinition cross_reader(
	const std::string& id,
	bool publishes_demand)
{
	if (publishes_demand)
	{
		return {
			id,
			SystemGroup::Control,
			[](SystemSetup& setup)
			{
				setup.read(AircraftDataKeys::kPrimaryControlPosition);
				setup.publish(
					AircraftDataKeys::kFlightControlDemand,
					demand(kInitialDemand));
			},
			[](const AircraftDataSnapshot& snapshot, SystemResult& result)
			{
				const double source = snapshot.read(
					AircraftDataKeys::kPrimaryControlPosition).elevator;
				result.publish(
					AircraftDataKeys::kFlightControlDemand,
					demand(source + kDemandOffset));
			}
		};
	}
	return {
		id,
		SystemGroup::Control,
		[](SystemSetup& setup)
		{
			setup.read(AircraftDataKeys::kFlightControlDemand);
			setup.publish(
				AircraftDataKeys::kPrimaryControlPosition,
				position(kInitialCrossPosition));
		},
		[](const AircraftDataSnapshot& snapshot, SystemResult& result)
		{
			const double source =
				snapshot.read(AircraftDataKeys::kFlightControlDemand).pitch;
			result.publish(
				AircraftDataKeys::kPrimaryControlPosition,
				position(source + kPositionOffset));
		}
	};
}

void expect_order_independent_result(
	Tests::Context& context,
	const AircraftDataSnapshot& output)
{
	TEST_EXPECT_NEAR(
		context,
		output.read(AircraftDataKeys::kFlightControlDemand).pitch,
		kInitialCrossPosition + kDemandOffset,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		output.read(AircraftDataKeys::kPrimaryControlPosition).elevator,
		kInitialDemand + kPositionOffset,
		kTolerance);
}

void test_catalog_order_does_not_change_group_result(Tests::Context& context)
{
	const SystemEntry first = entry(cross_reader("demand", true));
	const SystemEntry second = entry(cross_reader("position", false));
	SystemPipeline forward(flight_setup(), { first, second });
	SystemPipeline reverse(flight_setup(), { second, first });
	expect_order_independent_result(context, step_pipeline(forward));
	expect_order_independent_result(context, step_pipeline(reverse));
}

void test_phase_three_generated_catalog(Tests::Context& context)
{
	SystemPipeline pipeline(flight_setup());
	TEST_EXPECT(context, pipeline.system_count() == kPhaseThreeCatalogSize);
	const AircraftDataSnapshot output = step_pipeline(pipeline);
	TEST_EXPECT_NEAR(
		context,
		output.read(AircraftDataKeys::kPilotControlState).pitch,
		kNeutralValue,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		output.read(AircraftDataKeys::kPrimaryControlPosition).elevator,
		kNeutralValue,
		kTolerance);
	TEST_EXPECT(
		context,
		output.read(AircraftDataKeys::kFuelData).total_fuel_flow >=
			kNeutralValue);
	TEST_EXPECT_NEAR(
		context,
		output.read(AircraftDataKeys::kAirframeIntegrity).left_wing,
		kFullIntegrity,
		kTolerance);
}

void test_factory_receives_start_mode(Tests::Context& context)
{
	auto received_mode = std::make_shared<StartMode>(StartMode::ColdGround);
	SystemEntry system = {
		"context_reader",
		SystemGroup::Control,
		[received_mode](const FlightSetupContext& setup)
		{
			*received_mode = setup.start_mode;
			return std::make_unique<CallbackSystem>(SystemDefinition{
				"context_reader",
				SystemGroup::Control,
				[](SystemSetup&) {},
				no_step()
			});
		}
	};
	SystemPipeline pipeline(flight_setup(), { std::move(system) });
	TEST_EXPECT(context, *received_mode == StartMode::HotGround);
}
}

void run_system_pipeline_data_tests(Tests::Context& context)
{
	test_missing_provider_fails(context);
	test_type_mismatch_fails(context);
	test_required_initial_value_fails(context);
	test_optional_initial_value_is_explicit(context);
	test_duplicate_writer_fails(context);
	test_same_group_reads_fixed_snapshot(context);
	test_equipment_reads_control_commit(context);
	test_failed_equipment_keeps_previous_frame(context);
	test_missing_new_value_retains_last_commit(context);
	test_pending_storage_does_not_leak(context);
	test_catalog_order_does_not_change_group_result(context);
	test_phase_three_generated_catalog(context);
	test_factory_receives_start_mode(context);
}
