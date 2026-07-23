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

void test_fuel_flow_matches_consumption_rate(Tests::Context& context)
{
	Systems::FuelSystem fuel;
	Systems::set_external_fuel(fuel, { 1, 10.0, {} });
	const Systems::FuelSystemConfig config = { 3.0 };
	const Systems::FuelConsumptionInput input = {
		0.2, 0.5, 1.0, 0.0, 0.0, 2.0
	};
	Systems::simulate_fuel_consumption(fuel, config, input);
	TEST_EXPECT_NEAR(context, fuel.total_fuel_flow, 2.5, kTolerance);
	TEST_EXPECT_NEAR(
		context,
		fuel.fuel_consumption_since_last_time,
		0.5,
		kTolerance);
}

void test_external_fuel_is_aggregated_by_station(Tests::Context& context)
{
	Systems::FuelSystem fuel;
	Systems::set_external_fuel(fuel, { 1, 10.0, { 1.0, 0.0, 0.0 } });
	Systems::set_external_fuel(fuel, { 2, 20.0, { 2.0, 0.0, 0.0 } });
	TEST_EXPECT_NEAR(context, Systems::get_external_fuel(fuel), 30.0, kTolerance);
	Systems::set_external_fuel(fuel, { 1, 4.0, { 1.0, 0.0, 0.0 } });
	TEST_EXPECT_NEAR(context, Systems::get_external_fuel(fuel), 24.0, kTolerance);
	Systems::set_external_fuel(fuel, { 2, 0.0, {} });
	TEST_EXPECT_NEAR(context, Systems::get_external_fuel(fuel), 4.0, kTolerance);
}

void test_consumption_crosses_external_fuel_boundary(Tests::Context& context)
{
	Systems::FuelSystem fuel;
	fuel.internal_fuel = 10.0;
	Systems::set_external_fuel(fuel, { 1, 0.25, {} });
	const Systems::FuelSystemConfig config = { 3.0 };
	const Systems::FuelConsumptionInput input = {
		0.2, 0.5, 1.0, 0.0, 0.0, 2.0
	};
	Systems::simulate_fuel_consumption(fuel, config, input);
	TEST_EXPECT_NEAR(context, fuel.external_fuel, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, fuel.internal_fuel, 9.75, kTolerance);
	TEST_EXPECT_NEAR(
		context, fuel.fuel_consumption_since_last_time, 0.5, kTolerance);
}

void test_transient_reset_preserves_prepared_fuel(Tests::Context& context)
{
	Systems::FuelSystem fuel;
	fuel.internal_fuel = 7.0;
	Systems::set_external_fuel(fuel, { 3, 5.0, {} });
	fuel.total_fuel_flow = 2.0;
	fuel.fuel_consumption_since_last_time = 1.0;
	Systems::reset_fuel_transient_state(fuel);
	TEST_EXPECT_NEAR(context, fuel.internal_fuel, 7.0, kTolerance);
	TEST_EXPECT_NEAR(context, fuel.external_fuel, 5.0, kTolerance);
	TEST_EXPECT_NEAR(context, fuel.total_fuel_flow, 0.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, fuel.fuel_consumption_since_last_time, 0.0, kTolerance);
}
}

void run_mass_delta_tests(Tests::Context& context)
{
	test_pending_mass_delta_is_consumed_once(context);
	test_fuel_flow_matches_consumption_rate(context);
	test_external_fuel_is_aggregated_by_station(context);
	test_consumption_crosses_external_fuel_boundary(context);
	test_transient_reset_preserves_prepared_fuel(context);
}
