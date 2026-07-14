#include "TestHarness.h"

#include "Systems/FuelSystem.h"

namespace
{
constexpr double kTolerance = 1e-9;

void test_pending_mass_delta_is_consumed_once(Tests::Context& context)
{
	Systems::FuelSystem fuel;
	fuel.fuel_consumption_since_last_time = 12.5;
	const Systems::FuelMassDeltaResult first = Systems::take_fuel_mass_delta(fuel);
	TEST_EXPECT(context, first.available);
	TEST_EXPECT_NEAR(context, first.delta.mass, 12.5, kTolerance);
	TEST_EXPECT_NEAR(context, first.delta.position.x, -1.0, kTolerance);
	TEST_EXPECT_NEAR(context, first.delta.position.y, 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, first.delta.position.z, 0.0, kTolerance);

	const Systems::FuelMassDeltaResult second = Systems::take_fuel_mass_delta(fuel);
	TEST_EXPECT(context, !second.available);
	TEST_EXPECT_NEAR(context, fuel.fuel_consumption_since_last_time, 0.0, kTolerance);
}
}

void run_mass_delta_tests(Tests::Context& context)
{
	test_pending_mass_delta_is_consumed_once(context);
}
