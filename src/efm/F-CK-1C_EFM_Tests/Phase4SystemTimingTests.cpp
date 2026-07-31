#include "Fck1cEfmTestFixture.h"
#include "SystemPipelineTestFixture.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <vector>

namespace
{
using namespace Core;
using namespace Core::Systems;
using namespace SystemPipelineTest;

constexpr std::size_t kComparisonFrameCount = 6;
constexpr double kPitchCommand = 0.3;
constexpr double kYawCommand = -0.2;
constexpr double kThrottleCommand = 0.8;
constexpr double kGearUpCommand = 0.0;
constexpr double kActiveCommand = 1.0;
constexpr double kInitialFuel = 100.0;
constexpr double kFrameDt = 0.02;
constexpr double kHighSpeedObservation = 100.0;
constexpr double kNeutralSteering = 0.0;
constexpr double kTolerance = 1e-12;
constexpr std::uint32_t kSlowUpdateRateHz = 32;
constexpr std::size_t kExpectedOneSecondTicks = 64;
constexpr std::size_t kThirtyFpsFrames = 30;
constexpr std::size_t kSixtyFpsFrames = 60;
constexpr std::size_t kOneHundredFortyFourFpsFrames = 144;
constexpr std::uint32_t kLongRunUpdateRateHz = 3;
constexpr std::size_t kExpectedLongRunTicks = 10'800;
constexpr double kPublisherInitial = 10.0;
constexpr double kPublisherNext = 11.0;
constexpr double kPilotPitchCommand = 0.3;
constexpr SystemScheduledTime kOneSecond =
	std::chrono::seconds(1);
constexpr SystemScheduledTime kLongRunDuration =
	std::chrono::hours(1);
constexpr SystemScheduledTime kDurationDeclaredPeriod =
	std::chrono::milliseconds(10);

std::vector<double> engine_values(
	const EngineChannelData& engine)
{
	return {
		static_cast<double>(engine.switch_on),
		engine.throttle_input,
		engine.throttle_output,
		engine.power_readout,
		engine.afterburner_ratio,
		static_cast<double>(engine.afterburner_lit),
		engine.nozzle_aperture,
		engine.condition
	};
}

std::vector<double> landing_gear_values(
	const LandingGearData& gear)
{
	std::vector<double> values = {
		gear.position,
		gear.nose_wheel_steering,
		gear.brake_left,
		gear.brake_right,
		static_cast<double>(gear.any_weight_on_wheels),
		static_cast<double>(gear.on_ground)
	};
	for (std::size_t index = 0; index < gear.wheel_spin.size(); ++index)
	{
		const SuspensionWheelData& wheel = gear.suspension[index];
		values.insert(values.end(), {
			gear.wheel_radius[index],
			gear.wheel_spin[index],
			wheel.acting_force.x,
			wheel.acting_force.y,
			wheel.acting_force.z,
			wheel.compression,
			wheel.force_magnitude,
			static_cast<double>(wheel.weight_on_wheel)
		});
	}
	return values;
}

std::vector<double> published_values(
	const AircraftDataSnapshot& snapshot)
{
	const PilotControlState& pilot =
		snapshot.read(AircraftDataKeys::kPilotControlState);
	const FlightControlDemand& control =
		snapshot.read(AircraftDataKeys::kFlightControlDemand);
	const PrimaryControlPosition& primary =
		snapshot.read(AircraftDataKeys::kPrimaryControlPosition);
	const EngineControlDemand& engine_control =
		snapshot.read(AircraftDataKeys::kEngineControlDemand);
	const SecondaryControlPosition& secondary =
		snapshot.read(AircraftDataKeys::kSecondaryControlPosition);
	const EngineData& engines =
		snapshot.read(AircraftDataKeys::kEngineData);
	const FuelData& fuel = snapshot.read(AircraftDataKeys::kFuelData);
	const AirframeIntegrity& integrity =
		snapshot.read(AircraftDataKeys::kAirframeIntegrity);
	std::vector<double> values = {
		pilot.pitch, pilot.roll, pilot.yaw,
		control.pitch, control.roll, control.yaw,
		primary.elevator, primary.aileron, primary.rudder,
		engine_control.left_throttle, engine_control.right_throttle,
		secondary.flaps, secondary.slats, secondary.airbrake
	};
	const std::vector<double> landing = landing_gear_values(
		snapshot.read(AircraftDataKeys::kLandingGearData));
	values.insert(values.end(), landing.begin(), landing.end());
	const std::vector<double> left_engine = engine_values(engines.left);
	values.insert(values.end(), left_engine.begin(), left_engine.end());
	const std::vector<double> right_engine = engine_values(engines.right);
	values.insert(values.end(), right_engine.begin(), right_engine.end());
	values.insert(values.end(), {
		static_cast<double>(engines.thrust_inhibited),
		snapshot.read(AircraftDataKeys::kFuelDemand).flow_rate_kg_s,
		fuel.internal_fuel, fuel.external_fuel, fuel.total_fuel_flow,
		integrity.left_wing, integrity.right_wing, integrity.tail
	});
	return values;
}

void send_scenario_commands(SystemPipeline& pipeline)
{
	(void)pipeline.send({
		CommandId::SetPitchAxis, kPitchCommand });
	(void)pipeline.send({
		CommandId::SetYawAxis, kYawCommand });
	(void)pipeline.send({
		CommandId::SetCommonThrottleAxis,
		kThrottleCommand
	});
	(void)pipeline.send({
		CommandId::SetGear, kGearUpCommand });
	(void)pipeline.send({
		CommandId::SetFlapsAuto, kActiveCommand });
}

void test_production_output_ignores_catalog_entry_order(
	Tests::Context& context)
{
	auto forward_catalog = load_generated_system_catalog();
	auto reversed_catalog = load_generated_system_catalog();
	std::reverse(reversed_catalog.begin(), reversed_catalog.end());
	const FlightSetupContext setup = {
		StartMode::HotGround,
		{ kInitialFuel, {} }
	};
	SystemPipeline forward(setup, std::move(forward_catalog));
	SystemPipeline reversed(setup, std::move(reversed_catalog));
	send_scenario_commands(forward);
	send_scenario_commands(reversed);
	const FrameInput input = Tests::Fck1c::make_frame_input();
	const AircraftObservation observation;
	for (std::size_t frame = 0; frame < kComparisonFrameCount; ++frame)
	{
		TEST_EXPECT(
			context,
			published_values(SystemPipelineTest::step_pipeline(
				forward, input, observation)) ==
				published_values(SystemPipelineTest::step_pipeline(
					reversed, input, observation)));
	}
}

void test_equipment_reads_current_aircraft_observation(
	Tests::Context& context)
{
	SystemPipeline pipeline(SystemPipelineTest::flight_setup());
	(void)pipeline.send({
		CommandId::SetYawAxis,
		kYawCommand
	});
	FrameInput input;
	input.dt_s = kFrameDt;
	AircraftObservation observation;
	observation.speed_scalar = kHighSpeedObservation;
	observation.ground_speed = kHighSpeedObservation;
	const LandingGearData& gear = SystemPipelineTest::step_pipeline(
		pipeline, input, observation).read(
		AircraftDataKeys::kLandingGearData);
	TEST_EXPECT_NEAR(
		context,
		gear.nose_wheel_steering,
		kNeutralSteering,
		kTolerance);
}

SystemDefinition timed_recorder(
	const std::shared_ptr<std::vector<SystemStepContext>>& calls)
{
	SystemDefinition definition = {
		"timed",
		SystemGroup::Equipment,
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
	observation.speed_scalar = kHighSpeedObservation;
	(void)step_pipeline_to(
		pipeline,
		frame,
		observation,
		kTestUpdatePeriod - std::chrono::nanoseconds(1));
	TEST_EXPECT(context, calls->empty());
	TEST_EXPECT_NEAR(
		context,
		pipeline.snapshot().read(
			AircraftDataKeys::kAircraftObservation).speed_scalar,
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
		SystemGroup::Control,
		[](SystemSetup& setup)
		{
			setup.publish(
				AircraftDataKeys::kFlightControlDemand,
				demand(kPublisherInitial));
		},
		[calls](const AircraftDataView&, SystemResult& result)
		{
			++*calls;
			result.publish(
				AircraftDataKeys::kFlightControlDemand,
				demand(kPublisherInitial + *calls));
		}
	};
	definition.update_rate_hz = kSlowUpdateRateHz;
	return definition;
}

SystemDefinition fast_observer()
{
	return {
		"fast_observer",
		SystemGroup::Equipment,
		[](SystemSetup& setup)
		{
			setup.read(AircraftDataKeys::kFlightControlDemand);
			setup.publish(
				AircraftDataKeys::kPrimaryControlPosition,
				position(kNeutralAxis));
		},
		[](const AircraftDataView& aircraft, SystemResult& result)
		{
			result.publish(
				AircraftDataKeys::kPrimaryControlPosition,
				position(aircraft.read(
					AircraftDataKeys::kFlightControlDemand).pitch));
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
		.read(AircraftDataKeys::kPrimaryControlPosition).elevator;
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

void test_fcc_to_actuator_has_one_receiver_tick_delay(
	Tests::Context& context)
{
	SystemPipeline pipeline(flight_setup());
	(void)pipeline.send({
		CommandId::SetPitchAxis,
		kPilotPitchCommand
	});
	const FrameInput frame;
	const AircraftObservation observation;
	const AircraftDataSnapshot first =
		step_pipeline(pipeline, frame, observation);
	const double first_demand =
		first.read(AircraftDataKeys::kFlightControlDemand).pitch;
	TEST_EXPECT_NEAR(
		context,
		first.read(AircraftDataKeys::kPrimaryControlPosition).elevator,
		kNeutralAxis,
		kTolerance);
	const AircraftDataSnapshot second =
		step_pipeline(pipeline, frame, observation);
	TEST_EXPECT_NEAR(
		context,
		second.read(AircraftDataKeys::kPrimaryControlPosition).elevator,
		first_demand,
		kTolerance);
}
}

void run_phase_four_system_timing_tests(Tests::Context& context)
{
	test_production_output_ignores_catalog_entry_order(context);
	test_equipment_reads_current_aircraft_observation(context);
	test_first_tick_starts_after_one_full_period(context);
	test_large_host_interval_runs_every_due_tick(context);
	test_duration_declaration_supplies_its_period(context);
	test_long_run_keeps_exact_tick_count(context);
	test_different_time_buckets_commit_between_ticks(context);
	test_host_frame_partition_does_not_change_system_ticks(context);
	test_time_cannot_move_backward(context);
	test_fcc_to_actuator_has_one_receiver_tick_delay(context);
}
