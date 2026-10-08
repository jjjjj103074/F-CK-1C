#include "SystemPipelineTestFixture.h"

#include <chrono>
#include <cmath>
#include <memory>
#include <vector>

namespace
{
using namespace Core;
using namespace Core::Systems;
using namespace SystemPipelineTest;

constexpr double kTolerance = 1e-12;
constexpr double kHighSpeedObservation = 100.0;
constexpr std::uint32_t kSlowUpdateRateHz = 32;
constexpr std::size_t kExpectedOneSecondTicks = 64;
constexpr std::size_t kThirtyFpsFrames = 30;
constexpr std::size_t kSixtyFpsFrames = 60;
constexpr std::size_t kOneHundredFortyFourFpsFrames = 144;
constexpr std::uint32_t kLongRunUpdateRateHz = 3;
constexpr std::size_t kExpectedLongRunTicks = 10'800;
constexpr double kPublisherInitial = 10.0;
constexpr double kPublisherNext = 11.0;
constexpr SystemScheduledTime kOneSecond =
	std::chrono::seconds(1);
constexpr SystemScheduledTime kLongRunDuration =
	std::chrono::hours(1);
constexpr SystemScheduledTime kDurationDeclaredPeriod =
	std::chrono::milliseconds(10);

SystemDefinition timed_recorder(
	const std::shared_ptr<std::vector<SystemStepContext>>& calls)
{
	SystemDefinition definition = {
		"timed",
		[](SystemSetup&) {},
		no_step()
	};
	definition.timed_step = [calls](
		const SystemStepContext& step,
		const AircraftDataView&,
		SystemResult&)
	{
		calls->push_back(step);
	};
	return definition;
}

void test_first_tick_starts_after_one_full_period(
	Tests::Context& context)
{
	auto calls = std::make_shared<std::vector<SystemStepContext>>();
	SystemPipeline pipeline(
		flight_setup(), { entry(timed_recorder(calls)) });
	FrameInput frame;
	AircraftObservation observation;
	observation.true_airspeed_mps = kHighSpeedObservation;
	(void)step_pipeline_to(
		pipeline,
		frame,
		observation,
		kTestUpdatePeriod - std::chrono::nanoseconds(1));
	TEST_EXPECT(context, calls->empty());
	TEST_EXPECT_NEAR(
		context,
		pipeline.snapshot().read(
			AircraftDataKeys::kAircraftObservation).true_airspeed_mps,
		kHighSpeedObservation,
		kTolerance);
	(void)step_pipeline_to(
		pipeline, frame, observation, kTestUpdatePeriod);
	TEST_EXPECT(context, calls->size() == 1);
	TEST_EXPECT(context, calls->front().scheduled_time == kTestUpdatePeriod);
	TEST_EXPECT_NEAR(
		context,
		calls->front().dt_s,
		1.0 / static_cast<double>(kTestUpdateRateHz),
		kTolerance);
}

void test_large_host_interval_runs_every_due_tick(
	Tests::Context& context)
{
	auto calls = std::make_shared<std::vector<SystemStepContext>>();
	SystemPipeline pipeline(
		flight_setup(), { entry(timed_recorder(calls)) });
	const FrameInput frame;
	const AircraftObservation observation;
	(void)step_pipeline_to(pipeline, frame, observation, kOneSecond);
	TEST_EXPECT(context, calls->size() == kExpectedOneSecondTicks);
	TEST_EXPECT(context, calls->back().scheduled_time == kOneSecond);
}

void test_duration_declaration_supplies_its_period(
	Tests::Context& context)
{
	auto calls = std::make_shared<std::vector<SystemStepContext>>();
	SystemDefinition definition = timed_recorder(calls);
	definition.update_rate_hz = std::nullopt;
	definition.setup = [](SystemSetup& setup)
	{
		setup.update_period(kDurationDeclaredPeriod);
	};
	SystemPipeline pipeline(flight_setup(), { entry(definition) });
	const FrameInput frame;
	const AircraftObservation observation;
	(void)step_pipeline_to(
		pipeline, frame, observation, kDurationDeclaredPeriod);
	TEST_EXPECT(context, calls->size() == 1);
	TEST_EXPECT_NEAR(
		context,
		calls->front().dt_s,
		std::chrono::duration<double>(kDurationDeclaredPeriod).count(),
		kTolerance);
}

void test_long_run_keeps_exact_tick_count(Tests::Context& context)
{
	auto calls = std::make_shared<std::vector<SystemStepContext>>();
	SystemDefinition definition = timed_recorder(calls);
	definition.update_rate_hz = kLongRunUpdateRateHz;
	SystemPipeline pipeline(flight_setup(), { entry(definition) });
	const FrameInput frame;
	const AircraftObservation observation;
	(void)step_pipeline_to(
		pipeline, frame, observation, kLongRunDuration);
	TEST_EXPECT(context, calls->size() == kExpectedLongRunTicks);
	TEST_EXPECT(context, calls->back().scheduled_time == kLongRunDuration);
}

SystemDefinition slow_publisher()
{
	auto calls = std::make_shared<int>(0);
	SystemDefinition definition = {
		"slow_publisher",
		[](SystemSetup& setup)
		{
			setup.publish(
				AircraftDataKeys::kFlightControlActuatorCommand,
				actuator_command(kPublisherInitial));
		},
		[calls](const AircraftDataView&, SystemResult& result)
		{
			++*calls;
			result.publish(
				AircraftDataKeys::kFlightControlActuatorCommand,
				actuator_command(kPublisherInitial + *calls));
		}
	};
	definition.update_rate_hz = kSlowUpdateRateHz;
	return definition;
}

SystemDefinition fast_observer()
{
	return {
		"fast_observer",
		[](SystemSetup& setup)
		{
			setup.read(AircraftDataKeys::kFlightControlActuatorCommand);
			setup.publish(
				AircraftDataKeys::kFlightControlActuatorState,
				actuator_state(kNeutralAxis));
		},
		[](const AircraftDataView& aircraft, SystemResult& result)
		{
			result.publish(
				AircraftDataKeys::kFlightControlActuatorState,
				actuator_state(aircraft.read(
					AircraftDataKeys::kFlightControlActuatorCommand).symmetric_stabilator_demand_normalized));
		}
	};
}

double observed_position_at(
	SystemPipeline& pipeline,
	SystemScheduledTime target)
{
	const FrameInput frame;
	const AircraftObservation observation;
	return step_pipeline_to(pipeline, frame, observation, target)
		.read(AircraftDataKeys::kFlightControlActuatorState).symmetric_stabilator.position_rad;
}

void test_different_time_buckets_commit_between_ticks(
	Tests::Context& context)
{
	SystemPipeline pipeline(
		flight_setup(), { entry(slow_publisher()), entry(fast_observer()) });
	TEST_EXPECT_NEAR(
		context,
		observed_position_at(pipeline, kTestUpdatePeriod),
		kPublisherInitial,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		observed_position_at(pipeline, 2 * kTestUpdatePeriod),
		kPublisherInitial,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		observed_position_at(pipeline, 3 * kTestUpdatePeriod),
		kPublisherNext,
		kTolerance);
}

std::vector<SystemScheduledTime> make_host_targets(
	std::size_t frame_count)
{
	std::vector<SystemScheduledTime> result;
	result.reserve(frame_count);
	const long double nanoseconds =
		static_cast<long double>(kOneSecond.count());
	for (std::size_t frame = 1; frame <= frame_count; ++frame)
	{
		const long double target =
			nanoseconds * frame / frame_count;
		result.emplace_back(
			static_cast<SystemScheduledTime::rep>(std::llround(target)));
	}
	return result;
}

std::vector<SystemScheduledTime> run_partitioned_schedule(
	const std::vector<SystemScheduledTime>& targets)
{
	auto calls = std::make_shared<std::vector<SystemStepContext>>();
	SystemPipeline pipeline(
		flight_setup(), { entry(timed_recorder(calls)) });
	const FrameInput frame;
	const AircraftObservation observation;
	for (const SystemScheduledTime target : targets)
	{
		(void)step_pipeline_to(pipeline, frame, observation, target);
	}
	std::vector<SystemScheduledTime> result;
	result.reserve(calls->size());
	for (const SystemStepContext& call : *calls)
	{
		result.push_back(call.scheduled_time);
	}
	return result;
}

void test_host_frame_partition_does_not_change_system_ticks(
	Tests::Context& context)
{
	const std::vector<SystemScheduledTime> thirty_fps =
		run_partitioned_schedule(make_host_targets(kThirtyFpsFrames));
	const std::vector<SystemScheduledTime> sixty_fps =
		run_partitioned_schedule(make_host_targets(kSixtyFpsFrames));
	const std::vector<SystemScheduledTime> one_forty_four_fps =
		run_partitioned_schedule(
			make_host_targets(kOneHundredFortyFourFpsFrames));
	const std::vector<SystemScheduledTime> irregular =
		run_partitioned_schedule({
			std::chrono::milliseconds(1),
			std::chrono::milliseconds(17),
			std::chrono::milliseconds(52),
			std::chrono::milliseconds(101),
			std::chrono::milliseconds(333),
			std::chrono::milliseconds(777),
			kOneSecond
		});
	TEST_EXPECT(context, thirty_fps == sixty_fps);
	TEST_EXPECT(context, thirty_fps == one_forty_four_fps);
	TEST_EXPECT(context, thirty_fps == irregular);
	TEST_EXPECT(context, thirty_fps.size() == kExpectedOneSecondTicks);
}

void test_time_cannot_move_backward(Tests::Context& context)
{
	SystemPipeline pipeline(
		flight_setup(), { entry(timed_recorder(
			std::make_shared<std::vector<SystemStepContext>>())) });
	const FrameInput frame;
	const AircraftObservation observation;
	(void)step_pipeline_to(
		pipeline, frame, observation, kTestUpdatePeriod);
	TEST_EXPECT(
		context,
		action_throws([&pipeline, &frame, &observation]()
		{
			(void)step_pipeline_to(
				pipeline, frame, observation, SystemScheduledTime{});
		}));
}

}

void run_phase_four_system_timing_tests(Tests::Context& context)
{
	test_first_tick_starts_after_one_full_period(context);
	test_large_host_interval_runs_every_due_tick(context);
	test_duration_declaration_supplies_its_period(context);
	test_long_run_keeps_exact_tick_count(context);
	test_different_time_buckets_commit_between_ticks(context);
	test_host_frame_partition_does_not_change_system_ticks(context);
	test_time_cannot_move_backward(context);
}
