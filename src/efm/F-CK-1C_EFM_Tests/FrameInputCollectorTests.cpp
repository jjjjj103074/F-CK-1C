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
Core::CockpitObservation make_cockpit_observation()
{
	Core::CockpitObservation observation;
	observation.magnetic_heading.status = {
		true,
		11,
		Core::ObservationInvalidReason::None
	};
	observation.magnetic_heading.magnetic_heading_deg = 1.25;
	observation.pressure_altitude.status = {
		true,
		15,
		Core::ObservationInvalidReason::None
	};
	observation.pressure_altitude.pressure_altitude_ft = 5200.0;
	observation.radar.status = {
		true,
		12,
		Core::ObservationInvalidReason::None
	};
	observation.radar.stt_range_m = 4500.0;
	observation.ir_seeker.status = {
		true,
		13,
		Core::ObservationInvalidReason::None
	};
	observation.weapon_stations.status = {
		true,
		14,
		Core::ObservationInvalidReason::None
	};
	observation.weapon_stations.aim9_count = 2;
	return observation;
}

Common::Vec3 make_vec(double base)
{
	return Common::Vec3(base + 1.0, base + 2.0, base + 3.0);
}

Core::AtmosphereInput make_atmosphere(double base)
{
	Core::AtmosphereInput sample;
	sample.altitude_asl_m = base + 1.0;
	sample.temperature_k = base + 2.0;
	sample.speed_of_sound_mps = base + 3.0;
	sample.density_kg_m3 = base + 4.0;
	sample.pressure_pa = base + 5.0;
	sample.wind_world_mps = make_vec(base + 10.0);
	return sample;
}

Core::SurfaceInput make_surface(double base)
{
	Core::SurfaceInput sample;
	sample.surface_height_m = base + 1.0;
	sample.surface_height_with_objects_m = base + 2.0;
	sample.surface_type = kTestSurfaceType;
	sample.normal_world_unit = make_vec(base + 10.0);
	return sample;
}

Core::MassStateInput make_mass(double base)
{
	Core::MassStateInput sample;
	sample.mass_kg = base + 1.0;
	sample.center_of_mass_body_m = make_vec(base + 10.0);
	sample.moment_of_inertia_body_kg_m2 = make_vec(base + 20.0);
	return sample;
}

Core::WorldKinematicsInput make_world_kinematics(double base)
{
	Core::WorldKinematicsInput sample;
	sample.acceleration_world_mps2 = make_vec(base + 10.0);
	sample.velocity_world_mps = make_vec(base + 20.0);
	sample.position_world_m = make_vec(base + 30.0);
	sample.angular_acceleration_world_rad_s2 = make_vec(base + 40.0);
	sample.angular_velocity_world_rad_s = make_vec(base + 50.0);
	sample.orientation = { base + 61.0, base + 62.0, base + 63.0, base + 64.0 };
	return sample;
}

Core::BodyKinematicsInput make_body_kinematics(double base)
{
	Core::BodyKinematicsInput sample;
	sample.acceleration_body_mps2 = make_vec(base + 10.0);
	sample.velocity_body_mps = make_vec(base + 20.0);
	sample.wind_velocity_body_mps = make_vec(base + 30.0);
	sample.angular = {
		base + 41.0, base + 42.0, base + 43.0,
		base + 51.0, base + 52.0, base + 53.0
	};
	sample.world_yaw_rad = base + 61.0;
	sample.pitch_rad = base + 62.0;
	sample.roll_rad = base + 63.0;
	sample.angle_of_attack_rad = base + 64.0;
	sample.angle_of_slide_rad = base + 65.0;
	return sample;
}

Core::SuspensionFeedbackInput make_suspension(int index, double base)
{
	Core::SuspensionFeedbackInput sample;
	sample.index = index;
	sample.acting_force_body_n = make_vec(base + 10.0);
	sample.acting_force_point_body_m = make_vec(base + 20.0);
	sample.integrity_factor_0_1 = base + 31.0;
	sample.compression_m = base + 32.0;
	sample.wheel_speed_x_mps = base + 33.0;
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
	TEST_EXPECT_NEAR(context, actual.altitude_asl_m, expected.altitude_asl_m, kTolerance);
	TEST_EXPECT_NEAR(context, actual.temperature_k, expected.temperature_k, kTolerance);
	TEST_EXPECT_NEAR(context, actual.speed_of_sound_mps, expected.speed_of_sound_mps, kTolerance);
	TEST_EXPECT_NEAR(context, actual.density_kg_m3, expected.density_kg_m3, kTolerance);
	TEST_EXPECT_NEAR(context, actual.pressure_pa, expected.pressure_pa, kTolerance);
	expect_vec(context, actual.wind_world_mps, expected.wind_world_mps);
}

void expect_surface(Tests::Context& context, const Core::SurfaceInput& actual)
{
	const Core::SurfaceInput expected = make_surface(kSurfaceBase);
	TEST_EXPECT_NEAR(context, actual.surface_height_m, expected.surface_height_m, kTolerance);
	TEST_EXPECT_NEAR(
		context,
		actual.surface_height_with_objects_m,
		expected.surface_height_with_objects_m,
		kTolerance);
	TEST_EXPECT(context, actual.surface_type == expected.surface_type);
	expect_vec(context, actual.normal_world_unit, expected.normal_world_unit);
}

void expect_mass(Tests::Context& context, const Core::MassStateInput& actual)
{
	const Core::MassStateInput expected = make_mass(kMassBase);
	TEST_EXPECT_NEAR(context, actual.mass_kg, expected.mass_kg, kTolerance);
	expect_vec(context, actual.center_of_mass_body_m, expected.center_of_mass_body_m);
	expect_vec(context, actual.moment_of_inertia_body_kg_m2, expected.moment_of_inertia_body_kg_m2);
}

void expect_world_kinematics(
	Tests::Context& context,
	const Core::WorldKinematicsInput& actual)
{
	const Core::WorldKinematicsInput expected = make_world_kinematics(kWorldKinematicsBase);
	expect_vec(context, actual.acceleration_world_mps2, expected.acceleration_world_mps2);
	expect_vec(context, actual.velocity_world_mps, expected.velocity_world_mps);
	expect_vec(context, actual.position_world_m, expected.position_world_m);
	expect_vec(context, actual.angular_acceleration_world_rad_s2, expected.angular_acceleration_world_rad_s2);
	expect_vec(context, actual.angular_velocity_world_rad_s, expected.angular_velocity_world_rad_s);
	TEST_EXPECT_NEAR(context, actual.orientation.x, expected.orientation.x, kTolerance);
	TEST_EXPECT_NEAR(context, actual.orientation.y, expected.orientation.y, kTolerance);
	TEST_EXPECT_NEAR(context, actual.orientation.z, expected.orientation.z, kTolerance);
	TEST_EXPECT_NEAR(context, actual.orientation.w, expected.orientation.w, kTolerance);
}

void expect_body_angular_kinematics(
	Tests::Context& context,
	const Core::BodyAngularKinematicsInput& actual,
	const Core::BodyAngularKinematicsInput& expected)
{
	TEST_EXPECT_NEAR(context, actual.roll_acceleration_rad_s2,
		expected.roll_acceleration_rad_s2, kTolerance);
	TEST_EXPECT_NEAR(context, actual.pitch_acceleration_rad_s2,
		expected.pitch_acceleration_rad_s2, kTolerance);
	TEST_EXPECT_NEAR(context, actual.yaw_acceleration_rad_s2,
		expected.yaw_acceleration_rad_s2, kTolerance);
	TEST_EXPECT_NEAR(context, actual.roll_rate_rad_s,
		expected.roll_rate_rad_s, kTolerance);
	TEST_EXPECT_NEAR(context, actual.pitch_rate_rad_s,
		expected.pitch_rate_rad_s, kTolerance);
	TEST_EXPECT_NEAR(context, actual.yaw_rate_rad_s,
		expected.yaw_rate_rad_s, kTolerance);
}

void expect_body_kinematics(
	Tests::Context& context,
	const Core::BodyKinematicsInput& actual)
{
	const Core::BodyKinematicsInput expected = make_body_kinematics(kBodyKinematicsBase);
	expect_vec(context, actual.acceleration_body_mps2, expected.acceleration_body_mps2);
	expect_vec(context, actual.velocity_body_mps, expected.velocity_body_mps);
	expect_vec(context, actual.wind_velocity_body_mps, expected.wind_velocity_body_mps);
	expect_body_angular_kinematics(context, actual.angular, expected.angular);
	TEST_EXPECT_NEAR(
		context, actual.world_yaw_rad, expected.world_yaw_rad, kTolerance);
	TEST_EXPECT_NEAR(context, actual.pitch_rad, expected.pitch_rad, kTolerance);
	TEST_EXPECT_NEAR(context, actual.roll_rad, expected.roll_rad, kTolerance);
	TEST_EXPECT_NEAR(
		context, actual.angle_of_attack_rad,
		expected.angle_of_attack_rad, kTolerance);
	TEST_EXPECT_NEAR(
		context, actual.angle_of_slide_rad,
		expected.angle_of_slide_rad, kTolerance);
}

void expect_suspension(
	Tests::Context& context,
	const Core::SuspensionFeedbackInput& actual,
	const Core::SuspensionFeedbackInput& expected)
{
	TEST_EXPECT(context, actual.index == expected.index);
	expect_vec(context, actual.acting_force_body_n, expected.acting_force_body_n);
	expect_vec(context, actual.acting_force_point_body_m, expected.acting_force_point_body_m);
	TEST_EXPECT_NEAR(context, actual.integrity_factor_0_1, expected.integrity_factor_0_1, kTolerance);
	TEST_EXPECT_NEAR(context, actual.compression_m, expected.compression_m, kTolerance);
	TEST_EXPECT_NEAR(context, actual.wheel_speed_x_mps, expected.wheel_speed_x_mps, kTolerance);
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
	collector.publish_cockpit_observation(make_cockpit_observation());
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
	TEST_EXPECT(context, input.cockpit.magnetic_heading.status.available);
	TEST_EXPECT(context, input.cockpit.magnetic_heading.status.revision == 11);
	TEST_EXPECT_NEAR(
		context,
		input.cockpit.magnetic_heading.magnetic_heading_deg,
		1.25,
		kTolerance);
	TEST_EXPECT(context, input.cockpit.radar.status.available);
	TEST_EXPECT(context, input.cockpit.radar.status.revision == 12);
	TEST_EXPECT_NEAR(context, input.cockpit.radar.stt_range_m, 4500.0, kTolerance);
	TEST_EXPECT(context, input.cockpit.ir_seeker.status.revision == 13);
	TEST_EXPECT(context, input.cockpit.weapon_stations.status.revision == 14);
	TEST_EXPECT(context, input.cockpit.weapon_stations.aim9_count == 2);
	TEST_EXPECT(context, input.cockpit.pressure_altitude.status.available);
	TEST_EXPECT(context, input.cockpit.pressure_altitude.status.revision == 15);
	TEST_EXPECT_NEAR(context,
		input.cockpit.pressure_altitude.pressure_altitude_ft,
		5200.0, kTolerance);
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
		expect_vec(context, input.suspension[index].acting_force_body_n, Common::Vec3());
		TEST_EXPECT_NEAR(context, input.suspension[index].compression_m, 0.0, kTolerance);
	}
	TEST_EXPECT(context, !input.cockpit.magnetic_heading.status.available);
	TEST_EXPECT(context, !input.cockpit.radar.status.available);
	TEST_EXPECT(context, !input.cockpit.pressure_altitude.status.available);
	TEST_EXPECT(context, input.cockpit.radar.status.revision == 0);
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

void test_suspension_values_are_sticky_but_freshness_is_per_frame(
	Tests::Context& context)
{
	DcsBridge::Internal::FrameInputCollector collector;
	const Core::SuspensionFeedbackInput first = make_suspension(0, kFirstSuspensionBase);
	const Core::SuspensionFeedbackInput second = make_suspension(1, kSecondSuspensionBase);
	TEST_EXPECT(context, collector.publish_suspension(first));
	TEST_EXPECT(context, collector.publish_suspension(second));
	Core::FrameInput input = collector.snapshot(kSnapshotDtSeconds);
	expect_suspension(context, input.suspension[0], first);
	expect_suspension(context, input.suspension[1], second);
	TEST_EXPECT(context, input.availability.suspension[0]);
	TEST_EXPECT(context, input.availability.suspension[1]);
	TEST_EXPECT(context, !input.availability.suspension[2]);
	input = collector.snapshot(kSnapshotDtSeconds);
	expect_suspension(context, input.suspension[0], first);
	expect_suspension(context, input.suspension[1], second);
	for (bool suspension_available : input.availability.suspension)
	{
		TEST_EXPECT(context, !suspension_available);
	}
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
	TEST_EXPECT(context, input.availability.suspension[0]);
	TEST_EXPECT(context, !input.availability.suspension[1]);
	TEST_EXPECT(context, !input.availability.suspension[2]);
}

bool same_vec(const Common::Vec3& left, const Common::Vec3& right)
{
	return left.x == right.x && left.y == right.y && left.z == right.z;
}

bool same_atmosphere(
	const Core::AtmosphereInput& left,
	const Core::AtmosphereInput& right)
{
	return left.altitude_asl_m == right.altitude_asl_m &&
		left.temperature_k == right.temperature_k &&
		left.speed_of_sound_mps == right.speed_of_sound_mps &&
		left.density_kg_m3 == right.density_kg_m3 &&
		left.pressure_pa == right.pressure_pa &&
		same_vec(left.wind_world_mps, right.wind_world_mps);
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
	test_suspension_values_are_sticky_but_freshness_is_per_frame(context);
	test_concurrent_publish_and_snapshot(context);
}
