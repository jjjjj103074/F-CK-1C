#include "TestHarness.h"

#include "DcsBridge/Internal/FrameInputCollector.h"

#include <atomic>
#include <thread>

namespace
{
constexpr double kTolerance = 1e-9;
constexpr int kRaceIterations = 20000;
constexpr int kConcurrentParticipants = 2;
constexpr int kAlternatingSamplePeriod = 2;
constexpr int kTestSurfaceType = 4;
constexpr double kSnapshotDtSeconds = 0.01;
constexpr double kCompleteSnapshotDtSeconds = 0.02;
constexpr double kAtmosphereBase = 100.0;
constexpr double kSurfaceBase = 200.0;
constexpr double kMassBase = 300.0;
constexpr double kWorldKinematicsBase = 400.0;
constexpr double kBodyKinematicsBase = 500.0;
constexpr double kSuspensionBase = 600.0;
constexpr double kSuspensionBaseStep = 100.0;
constexpr double kFirstSuspensionBase = 10.0;
constexpr double kSecondSuspensionBase = 20.0;
constexpr double kReplacementSuspensionBase = 30.0;
constexpr double kInvalidSuspensionBase = 40.0;
constexpr double kConcurrentOldBase = 1000.0;
constexpr double kConcurrentNewBase = 2000.0;
constexpr Core::AutopilotCommand kAutopilotCommand = {
	true, true, true, 0.1, 0.2, 0.3
};
constexpr Core::MaxPowerCommand kMaxPowerCommand = { 0.4, 0.5 };

Common::Vec3 make_vec(double base)
{
	return Common::Vec3(base + 1.0, base + 2.0, base + 3.0);
}

Core::AtmosphereInput make_atmosphere(double base)
{
	Core::AtmosphereInput sample;
	sample.altitude_asl = base + 1.0;
	sample.temperature = base + 2.0;
	sample.speed_of_sound = base + 3.0;
	sample.density = base + 4.0;
	sample.pressure = base + 5.0;
	sample.wind = make_vec(base + 10.0);
	return sample;
}

Core::SurfaceInput make_surface(double base)
{
	Core::SurfaceInput sample;
	sample.surface_height = base + 1.0;
	sample.surface_height_with_objects = base + 2.0;
	sample.surface_type = kTestSurfaceType;
	sample.normal = make_vec(base + 10.0);
	return sample;
}

Core::MassStateInput make_mass(double base)
{
	Core::MassStateInput sample;
	sample.mass = base + 1.0;
	sample.center_of_mass = make_vec(base + 10.0);
	sample.moment_of_inertia = make_vec(base + 20.0);
	return sample;
}

Core::WorldKinematicsInput make_world_kinematics(double base)
{
	Core::WorldKinematicsInput sample;
	sample.acceleration = make_vec(base + 10.0);
	sample.velocity = make_vec(base + 20.0);
	sample.position = make_vec(base + 30.0);
	sample.angular_acceleration = make_vec(base + 40.0);
	sample.angular_velocity = make_vec(base + 50.0);
	sample.orientation = { base + 61.0, base + 62.0, base + 63.0, base + 64.0 };
	return sample;
}

Core::BodyKinematicsInput make_body_kinematics(double base)
{
	Core::BodyKinematicsInput sample;
	sample.acceleration = make_vec(base + 10.0);
	sample.velocity = make_vec(base + 20.0);
	sample.wind_velocity = make_vec(base + 30.0);
	sample.angular_acceleration = make_vec(base + 40.0);
	sample.angular_velocity = make_vec(base + 50.0);
	sample.heading = base + 61.0;
	sample.pitch = base + 62.0;
	sample.roll = base + 63.0;
	sample.angle_of_attack = base + 64.0;
	sample.angle_of_slide = base + 65.0;
	return sample;
}

Core::SuspensionFeedbackInput make_suspension(int index, double base)
{
	Core::SuspensionFeedbackInput sample;
	sample.index = index;
	sample.acting_force = make_vec(base + 10.0);
	sample.acting_force_point = make_vec(base + 20.0);
	sample.integrity_factor = base + 31.0;
	sample.compression = base + 32.0;
	sample.wheel_speed_x = base + 33.0;
	return sample;
}

void expect_vec(
	Tests::Context& context,
	const Common::Vec3& actual,
	const Common::Vec3& expected)
{
	TEST_EXPECT_NEAR(context, actual.x, expected.x, kTolerance);
	TEST_EXPECT_NEAR(context, actual.y, expected.y, kTolerance);
	TEST_EXPECT_NEAR(context, actual.z, expected.z, kTolerance);
}

void expect_atmosphere(Tests::Context& context, const Core::AtmosphereInput& actual)
{
	const Core::AtmosphereInput expected = make_atmosphere(kAtmosphereBase);
	TEST_EXPECT_NEAR(context, actual.altitude_asl, expected.altitude_asl, kTolerance);
	TEST_EXPECT_NEAR(context, actual.temperature, expected.temperature, kTolerance);
	TEST_EXPECT_NEAR(context, actual.speed_of_sound, expected.speed_of_sound, kTolerance);
	TEST_EXPECT_NEAR(context, actual.density, expected.density, kTolerance);
	TEST_EXPECT_NEAR(context, actual.pressure, expected.pressure, kTolerance);
	expect_vec(context, actual.wind, expected.wind);
}

void expect_surface(Tests::Context& context, const Core::SurfaceInput& actual)
{
	const Core::SurfaceInput expected = make_surface(kSurfaceBase);
	TEST_EXPECT_NEAR(context, actual.surface_height, expected.surface_height, kTolerance);
	TEST_EXPECT_NEAR(
		context,
		actual.surface_height_with_objects,
		expected.surface_height_with_objects,
		kTolerance);
	TEST_EXPECT(context, actual.surface_type == expected.surface_type);
	expect_vec(context, actual.normal, expected.normal);
}

void expect_mass(Tests::Context& context, const Core::MassStateInput& actual)
{
	const Core::MassStateInput expected = make_mass(kMassBase);
	TEST_EXPECT_NEAR(context, actual.mass, expected.mass, kTolerance);
	expect_vec(context, actual.center_of_mass, expected.center_of_mass);
	expect_vec(context, actual.moment_of_inertia, expected.moment_of_inertia);
}

void expect_world_kinematics(
	Tests::Context& context,
	const Core::WorldKinematicsInput& actual)
{
	const Core::WorldKinematicsInput expected = make_world_kinematics(kWorldKinematicsBase);
	expect_vec(context, actual.acceleration, expected.acceleration);
	expect_vec(context, actual.velocity, expected.velocity);
	expect_vec(context, actual.position, expected.position);
	expect_vec(context, actual.angular_acceleration, expected.angular_acceleration);
	expect_vec(context, actual.angular_velocity, expected.angular_velocity);
	TEST_EXPECT_NEAR(context, actual.orientation.x, expected.orientation.x, kTolerance);
	TEST_EXPECT_NEAR(context, actual.orientation.y, expected.orientation.y, kTolerance);
	TEST_EXPECT_NEAR(context, actual.orientation.z, expected.orientation.z, kTolerance);
	TEST_EXPECT_NEAR(context, actual.orientation.w, expected.orientation.w, kTolerance);
}

void expect_body_kinematics(
	Tests::Context& context,
	const Core::BodyKinematicsInput& actual)
{
	const Core::BodyKinematicsInput expected = make_body_kinematics(kBodyKinematicsBase);
	expect_vec(context, actual.acceleration, expected.acceleration);
	expect_vec(context, actual.velocity, expected.velocity);
	expect_vec(context, actual.wind_velocity, expected.wind_velocity);
	expect_vec(context, actual.angular_acceleration, expected.angular_acceleration);
	expect_vec(context, actual.angular_velocity, expected.angular_velocity);
	TEST_EXPECT_NEAR(context, actual.heading, expected.heading, kTolerance);
	TEST_EXPECT_NEAR(context, actual.pitch, expected.pitch, kTolerance);
	TEST_EXPECT_NEAR(context, actual.roll, expected.roll, kTolerance);
	TEST_EXPECT_NEAR(context, actual.angle_of_attack, expected.angle_of_attack, kTolerance);
	TEST_EXPECT_NEAR(context, actual.angle_of_slide, expected.angle_of_slide, kTolerance);
}

void expect_suspension(
	Tests::Context& context,
	const Core::SuspensionFeedbackInput& actual,
	const Core::SuspensionFeedbackInput& expected)
{
	TEST_EXPECT(context, actual.index == expected.index);
	expect_vec(context, actual.acting_force, expected.acting_force);
	expect_vec(context, actual.acting_force_point, expected.acting_force_point);
	TEST_EXPECT_NEAR(context, actual.integrity_factor, expected.integrity_factor, kTolerance);
	TEST_EXPECT_NEAR(context, actual.compression, expected.compression, kTolerance);
	TEST_EXPECT_NEAR(context, actual.wheel_speed_x, expected.wheel_speed_x, kTolerance);
}

void publish_complete_input(DcsBridge::Internal::FrameInputCollector& collector)
{
	collector.publish_atmosphere(make_atmosphere(kAtmosphereBase));
	collector.publish_surface(make_surface(kSurfaceBase));
	collector.publish_mass(make_mass(kMassBase));
	collector.publish_world_kinematics(make_world_kinematics(kWorldKinematicsBase));
	collector.publish_body_kinematics(make_body_kinematics(kBodyKinematicsBase));
	for (int index = 0; index < static_cast<int>(Core::kFrameSuspensionWheelCount); ++index)
	{
		collector.publish_suspension(
			make_suspension(index, kSuspensionBase + index * kSuspensionBaseStep));
	}
	collector.publish_autopilot(kAutopilotCommand);
	collector.publish_max_power(kMaxPowerCommand);
}

void expect_complete_availability(
	Tests::Context& context,
	const Core::FrameDataAvailability& availability)
{
	TEST_EXPECT(context, availability.atmosphere);
	TEST_EXPECT(context, availability.surface);
	TEST_EXPECT(context, availability.mass);
	TEST_EXPECT(context, availability.world_kinematics);
	TEST_EXPECT(context, availability.body_kinematics);
	for (bool suspension_available : availability.suspension)
	{
		TEST_EXPECT(context, suspension_available);
	}
}

void expect_complete_input(Tests::Context& context, const Core::FrameInput& input)
{
	TEST_EXPECT_NEAR(context, input.dt_s, kCompleteSnapshotDtSeconds, kTolerance);
	expect_complete_availability(context, input.availability);
	expect_atmosphere(context, input.atmosphere);
	expect_surface(context, input.surface);
	expect_mass(context, input.mass);
	expect_world_kinematics(context, input.world_kinematics);
	expect_body_kinematics(context, input.body_kinematics);
	for (int index = 0; index < static_cast<int>(Core::kFrameSuspensionWheelCount); ++index)
	{
		expect_suspension(
			context,
			input.suspension[index],
			make_suspension(index, kSuspensionBase + index * kSuspensionBaseStep));
	}
	TEST_EXPECT(context, input.autopilot.master);
	TEST_EXPECT(context, input.autopilot.bypass);
	TEST_EXPECT(context, input.autopilot.auto_throttle_engaged);
	TEST_EXPECT_NEAR(
		context, input.autopilot.pitch_command, kAutopilotCommand.pitch_command, kTolerance);
	TEST_EXPECT_NEAR(
		context, input.autopilot.roll_command, kAutopilotCommand.roll_command, kTolerance);
	TEST_EXPECT_NEAR(
		context,
		input.autopilot.throttle_command,
		kAutopilotCommand.throttle_command,
		kTolerance);
	TEST_EXPECT_NEAR(context, input.max_power.ready, kMaxPowerCommand.ready, kTolerance);
	TEST_EXPECT_NEAR(context, input.max_power.value, kMaxPowerCommand.value, kTolerance);
}

void expect_reset_input(Tests::Context& context, const Core::FrameInput& input)
{
	TEST_EXPECT_NEAR(context, input.dt_s, kSnapshotDtSeconds, kTolerance);
	TEST_EXPECT(context, !input.availability.atmosphere);
	TEST_EXPECT(context, !input.availability.surface);
	TEST_EXPECT(context, !input.availability.mass);
	TEST_EXPECT(context, !input.availability.world_kinematics);
	TEST_EXPECT(context, !input.availability.body_kinematics);
	for (int index = 0; index < static_cast<int>(Core::kFrameSuspensionWheelCount); ++index)
	{
		TEST_EXPECT(context, !input.availability.suspension[index]);
		TEST_EXPECT(context, input.suspension[index].index == index);
		expect_vec(context, input.suspension[index].acting_force, Common::Vec3());
		TEST_EXPECT_NEAR(context, input.suspension[index].compression, 0.0, kTolerance);
	}
	TEST_EXPECT(context, !input.autopilot.master);
	TEST_EXPECT(context, !input.autopilot.bypass);
	TEST_EXPECT(context, !input.autopilot.auto_throttle_engaged);
	TEST_EXPECT_NEAR(context, input.max_power.ready, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, input.max_power.value, 1.0, kTolerance);
}

void test_complete_publish_and_snapshot(Tests::Context& context)
{
	DcsBridge::Internal::FrameInputCollector collector;
	publish_complete_input(collector);
	expect_complete_input(context, collector.snapshot(kCompleteSnapshotDtSeconds));
}

void test_missing_categories_and_reset(Tests::Context& context)
{
	DcsBridge::Internal::FrameInputCollector collector;
	expect_reset_input(context, collector.snapshot(kSnapshotDtSeconds));
	publish_complete_input(collector);
	collector.reset();
	expect_reset_input(context, collector.snapshot(kSnapshotDtSeconds));
}

void test_sticky_suspension_and_invalid_index(Tests::Context& context)
{
	DcsBridge::Internal::FrameInputCollector collector;
	const Core::SuspensionFeedbackInput first = make_suspension(0, kFirstSuspensionBase);
	const Core::SuspensionFeedbackInput second = make_suspension(1, kSecondSuspensionBase);
	TEST_EXPECT(context, collector.publish_suspension(first));
	TEST_EXPECT(context, collector.publish_suspension(second));
	Core::FrameInput input = collector.snapshot(kSnapshotDtSeconds);
	expect_suspension(context, input.suspension[0], first);
	expect_suspension(context, input.suspension[1], second);
	TEST_EXPECT(context, !input.availability.suspension[2]);
	const Core::SuspensionFeedbackInput replacement =
		make_suspension(0, kReplacementSuspensionBase);
	TEST_EXPECT(context, collector.publish_suspension(replacement));
	TEST_EXPECT(
		context,
		!collector.publish_suspension(make_suspension(-1, kInvalidSuspensionBase)));
	TEST_EXPECT(
		context,
		!collector.publish_suspension(make_suspension(
			static_cast<int>(Core::kFrameSuspensionWheelCount),
			kInvalidSuspensionBase)));
	input = collector.snapshot(kSnapshotDtSeconds);
	expect_suspension(context, input.suspension[0], replacement);
	expect_suspension(context, input.suspension[1], second);
}

bool same_vec(const Common::Vec3& left, const Common::Vec3& right)
{
	return left.x == right.x && left.y == right.y && left.z == right.z;
}

bool same_atmosphere(
	const Core::AtmosphereInput& left,
	const Core::AtmosphereInput& right)
{
	return left.altitude_asl == right.altitude_asl &&
		left.temperature == right.temperature &&
		left.speed_of_sound == right.speed_of_sound &&
		left.density == right.density &&
		left.pressure == right.pressure &&
		same_vec(left.wind, right.wind);
}

void arrive_and_wait(std::atomic<int>& arrivals, int expected_arrivals)
{
	arrivals.fetch_add(1, std::memory_order_acq_rel);
	while (arrivals.load(std::memory_order_acquire) < expected_arrivals)
	{
		std::this_thread::yield();
	}
}

void test_concurrent_publish_and_snapshot(Tests::Context& context)
{
	DcsBridge::Internal::FrameInputCollector collector;
	const Core::AtmosphereInput old_sample = make_atmosphere(kConcurrentOldBase);
	const Core::AtmosphereInput new_sample = make_atmosphere(kConcurrentNewBase);
	collector.publish_atmosphere(old_sample);
	std::atomic<int> ready_count(0);
	std::atomic<int> complete_count(0);
	std::atomic<bool> complete_samples_only(true);
	std::thread writer([&]() {
		for (int index = 0; index < kRaceIterations; ++index)
		{
			const int expected_arrivals = (index + 1) * kConcurrentParticipants;
			arrive_and_wait(ready_count, expected_arrivals);
			collector.publish_atmosphere(
				index % kAlternatingSamplePeriod == 0 ? old_sample : new_sample);
			arrive_and_wait(complete_count, expected_arrivals);
		}
	});
	std::thread reader([&]() {
		for (int index = 0; index < kRaceIterations; ++index)
		{
			const int expected_arrivals = (index + 1) * kConcurrentParticipants;
			arrive_and_wait(ready_count, expected_arrivals);
			const Core::AtmosphereInput sample =
				collector.snapshot(kSnapshotDtSeconds).atmosphere;
			if (!same_atmosphere(sample, old_sample) && !same_atmosphere(sample, new_sample))
			{
				complete_samples_only.store(false, std::memory_order_relaxed);
			}
			arrive_and_wait(complete_count, expected_arrivals);
		}
	});
	writer.join();
	reader.join();
	TEST_EXPECT(context, complete_samples_only.load(std::memory_order_relaxed));
}
}

void run_frame_input_collector_tests(Tests::Context& context)
{
	test_complete_publish_and_snapshot(context);
	test_missing_categories_and_reset(context);
	test_sticky_suspension_and_invalid_index(context);
	test_concurrent_publish_and_snapshot(context);
}
