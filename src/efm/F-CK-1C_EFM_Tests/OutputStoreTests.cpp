#include "TestHarness.h"

#include "DcsBridge/Internal/OutputStore.h"

#include <atomic>
#include <thread>

namespace
{
constexpr int kRaceIterations = 20000;
constexpr int kMassRaceIterations = 5000;
constexpr int kAlternatingSamplePeriod = 2;
constexpr double kFirstMarker = 1000.0;
constexpr double kSecondMarker = 2000.0;
constexpr double kForceXOffset = 1.0;
constexpr double kForceYOffset = 2.0;
constexpr double kForceZOffset = 3.0;
constexpr double kThrustOffset = 4.0;
constexpr double kRudderOffset = 5.0;
constexpr double kWheelSpinOffset = 6.0;
constexpr double kCompressionOffset = 7.0;
constexpr double kFuelOffset = 8.0;
constexpr double kShakeOffset = 9.0;
constexpr double kFirstMassDelta = 10.0;
constexpr double kSecondMassDelta = 20.0;

Core::FrameOutput make_output(double marker)
{
	Core::FrameOutput output;
	output.simulation_time_s = marker;
	output.availability.atmosphere = marker == kFirstMarker;
	output.force_moment.force = {
		marker + kForceXOffset,
		marker + kForceYOffset,
		marker + kForceZOffset
	};
	output.engines[0].switch_on = marker == kFirstMarker;
	output.engines[1].thrust_force = marker + kThrustOffset;
	output.controls.rudder_command = marker + kRudderOffset;
	output.landing_gear.wheel_spin[2] = marker + kWheelSpinOffset;
	output.suspension.wheels[1].compression = marker + kCompressionOffset;
	output.fuel.total_fuel = marker + kFuelOffset;
	output.shake_amplitude = marker + kShakeOffset;
	return output;
}

Core::FrameOutput make_mass_output(double mass)
{
	Core::FrameOutput output;
	output.mass_effect.available = true;
	output.mass_effect.delta = {
		mass,
		{ mass + kForceXOffset, mass + kForceYOffset, mass + kForceZOffset },
		{ mass + kRudderOffset, mass + kWheelSpinOffset, mass + kCompressionOffset }
	};
	return output;
}

bool has_complete_primary_output(const Core::FrameOutput& output, double marker, bool first)
{
	return output.availability.atmosphere == first &&
		output.force_moment.force.x == marker + kForceXOffset &&
		output.force_moment.force.z == marker + kForceZOffset &&
		output.engines[0].switch_on == first &&
		output.engines[1].thrust_force == marker + kThrustOffset &&
		output.controls.rudder_command == marker + kRudderOffset;
}

bool has_complete_secondary_output(const Core::FrameOutput& output, double marker)
{
	return output.landing_gear.wheel_spin[2] == marker + kWheelSpinOffset &&
		output.suspension.wheels[1].compression == marker + kCompressionOffset &&
		output.fuel.total_fuel == marker + kFuelOffset &&
		output.shake_amplitude == marker + kShakeOffset;
}

bool is_complete_output(const Core::FrameOutput& output)
{
	const double marker = output.simulation_time_s;
	const bool first = marker == kFirstMarker;
	const bool known_marker = first || marker == kSecondMarker;
	return known_marker &&
		has_complete_primary_output(output, marker, first) &&
		has_complete_secondary_output(output, marker);
}

void test_lifecycle_and_immutable_copy(Tests::Context& context)
{
	DcsBridge::Internal::OutputStore store;
	TEST_EXPECT(context, !store.read());
	TEST_EXPECT(context, !store.is_released());
	Core::FrameOutput start = make_output(kFirstMarker);
	store.publish(start);
	start.fuel.total_fuel = -1.0;
	TEST_EXPECT(context, is_complete_output(*store.read()));
	const Core::FrameOutput next = make_output(kSecondMarker);
	TEST_EXPECT(context, store.read()->simulation_time_s == kFirstMarker);
	store.publish(next);
	TEST_EXPECT(context, is_complete_output(*store.read()));
	TEST_EXPECT(context, store.read()->simulation_time_s == kSecondMarker);
	store.mark_released();
	TEST_EXPECT(context, !store.read());
	TEST_EXPECT(context, store.is_released());
	store.publish(make_output(kFirstMarker));
	TEST_EXPECT(context, is_complete_output(*store.read()));
	TEST_EXPECT(context, !store.is_released());
}

void test_concurrent_publish_and_read(Tests::Context& context)
{
	DcsBridge::Internal::OutputStore store;
	store.publish(make_output(kFirstMarker));
	std::atomic<bool> complete_outputs_only(true);
	std::thread writer([&]() {
		for (int index = 0; index < kRaceIterations; ++index)
		{
			store.publish(make_output(
				index % kAlternatingSamplePeriod == 0 ? kFirstMarker : kSecondMarker));
		}
	});
	std::thread reader([&]() {
		for (int index = 0; index < kRaceIterations; ++index)
		{
			const std::optional<Core::FrameOutput> output = store.read();
			if (!output || !is_complete_output(*output))
			{
				complete_outputs_only.store(false, std::memory_order_relaxed);
			}
		}
	});
	writer.join();
	reader.join();
	TEST_EXPECT(context, complete_outputs_only.load(std::memory_order_relaxed));
}

void test_mass_delta_queue_preserves_order(Tests::Context& context)
{
	DcsBridge::Internal::OutputStore store;
	store.publish(make_mass_output(kFirstMassDelta));
	store.publish(make_mass_output(kSecondMassDelta));
	const Core::MassDeltaResult first = store.take_mass_delta();
	const Core::MassDeltaResult second = store.take_mass_delta();
	TEST_EXPECT(context, first.available);
	TEST_EXPECT(context, second.available);
	TEST_EXPECT_NEAR(context, first.delta.mass, kFirstMassDelta, 0.0);
	TEST_EXPECT_NEAR(context, second.delta.mass, kSecondMassDelta, 0.0);
	TEST_EXPECT_NEAR(
		context,
		first.delta.position.x,
		kFirstMassDelta + kForceXOffset,
		0.0);
	TEST_EXPECT_NEAR(
		context,
		second.delta.moment_of_inertia.z,
		kSecondMassDelta + kCompressionOffset,
		0.0);
	TEST_EXPECT(context, !store.take_mass_delta().available);
}

void test_flight_boundaries_clear_mass_delta_queue(Tests::Context& context)
{
	DcsBridge::Internal::OutputStore store;
	store.publish(make_mass_output(kFirstMassDelta));
	store.publish_start(make_output(kFirstMarker));
	TEST_EXPECT(context, !store.take_mass_delta().available);
	store.publish(make_mass_output(kSecondMassDelta));
	store.mark_released();
	TEST_EXPECT(context, !store.take_mass_delta().available);
}

void test_concurrent_mass_publish_and_take(Tests::Context& context)
{
	DcsBridge::Internal::OutputStore store;
	std::atomic<bool> ordered(true);
	std::thread writer([&store]() {
		for (int index = 1; index <= kMassRaceIterations; ++index)
		{
			store.publish(make_mass_output(static_cast<double>(index)));
		}
	});
	std::thread reader([&store, &ordered]() {
		int expected = 1;
		while (expected <= kMassRaceIterations)
		{
			const Core::MassDeltaResult result = store.take_mass_delta();
			if (!result.available)
			{
				std::this_thread::yield();
				continue;
			}
			if (result.delta.mass != static_cast<double>(expected))
			{
				ordered.store(false, std::memory_order_relaxed);
			}
			++expected;
		}
	});
	writer.join();
	reader.join();
	TEST_EXPECT(context, ordered.load(std::memory_order_relaxed));
	TEST_EXPECT(context, !store.take_mass_delta().available);
}
}

void run_output_store_tests(Tests::Context& context)
{
	test_lifecycle_and_immutable_copy(context);
	test_concurrent_publish_and_read(context);
	test_mass_delta_queue_preserves_order(context);
	test_flight_boundaries_clear_mass_delta_queue(context);
	test_concurrent_mass_publish_and_take(context);
}
