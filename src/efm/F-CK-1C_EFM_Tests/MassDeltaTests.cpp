#include "TestHarness.h"

#include "Core/Systems/Fuel/Fuel.h"

namespace
{
constexpr double kTolerance = 1e-9;
constexpr double kPendingConsumption = 12.5;
constexpr double kAvailableFuel = 20.0;
constexpr double kFlowRate = 2.5;
constexpr double kFrameDt = 0.2;
constexpr double kExpectedConsumption = 0.5;
constexpr double kExternalFuel = 10.0;
constexpr double kInternalFuel = 10.0;
constexpr double kBoundaryExternalFuel = 0.25;
constexpr double kRemainingInternalFuel = 9.75;
constexpr double kOneSecond = 1.0;
constexpr double kMassPositionX = -1.0;
constexpr double kMassPositionY = 1.0;
constexpr double kMassPositionZ = 0.0;
constexpr int kFirstFuelStation = 1;
constexpr int kSecondFuelStation = 2;
constexpr double kFirstStationFuel = 10.0;
constexpr double kSecondStationFuel = 20.0;
constexpr double kUpdatedFirstStationFuel = 4.0;
constexpr double kTotalStationFuel = 30.0;
constexpr double kUpdatedTotalStationFuel = 24.0;

void test_pending_mass_delta_is_consumed_once(Tests::Context& context)
{
	Core::Systems::Fuel fuel;
	fuel.set_internal_fuel(kAvailableFuel);
	fuel.step({ kPendingConsumption }, kOneSecond);
	const Systems::FuelMassDeltaResult first = fuel.take_mass_delta();
	TEST_EXPECT(context, first.available);
	TEST_EXPECT_NEAR(
		context, first.delta.mass, kPendingConsumption, kTolerance);
	TEST_EXPECT_NEAR(
		context, first.delta.position.x, kMassPositionX, kTolerance);
	TEST_EXPECT_NEAR(
		context, first.delta.position.y, kMassPositionY, kTolerance);
	TEST_EXPECT_NEAR(
		context, first.delta.position.z, kMassPositionZ, kTolerance);

	const Systems::FuelMassDeltaResult second = fuel.take_mass_delta();
	TEST_EXPECT(context, !second.available);
}

void test_fuel_consumes_registered_demand(Tests::Context& context)
{
	Core::Systems::Fuel fuel;
	fuel.set_external_fuel({ kFirstFuelStation, kExternalFuel, {} });
	const Core::FuelData& data =
		fuel.step({ kFlowRate }, kFrameDt);
	TEST_EXPECT_NEAR(
		context, data.total_fuel_flow, kFlowRate, kTolerance);
	const Systems::FuelMassDeltaResult delta = fuel.take_mass_delta();
	TEST_EXPECT(context, delta.available);
	TEST_EXPECT_NEAR(
		context,
		delta.delta.mass,
		kExpectedConsumption,
		kTolerance);
}

void test_external_fuel_is_aggregated_by_station(Tests::Context& context)
{
	Core::Systems::Fuel fuel;
	fuel.set_external_fuel({
		kFirstFuelStation,
		kFirstStationFuel,
		{ kMassPositionY, kMassPositionZ, kMassPositionZ }
	});
	fuel.set_external_fuel({
		kSecondFuelStation,
		kSecondStationFuel,
		{ kSecondFuelStation, kMassPositionZ, kMassPositionZ }
	});
	TEST_EXPECT_NEAR(
		context, fuel.external_fuel(), kTotalStationFuel, kTolerance);
	fuel.set_external_fuel({
		kFirstFuelStation,
		kUpdatedFirstStationFuel,
		{ kMassPositionY, kMassPositionZ, kMassPositionZ }
	});
	TEST_EXPECT_NEAR(
		context, fuel.external_fuel(), kUpdatedTotalStationFuel, kTolerance);
	fuel.set_external_fuel({ kSecondFuelStation, kMassPositionZ, {} });
	TEST_EXPECT_NEAR(
		context, fuel.external_fuel(), kUpdatedFirstStationFuel, kTolerance);
}

void test_consumption_crosses_external_fuel_boundary(Tests::Context& context)
{
	Core::Systems::Fuel fuel;
	fuel.set_internal_fuel(kInternalFuel);
	fuel.set_external_fuel({
		kFirstFuelStation, kBoundaryExternalFuel, {}
	});
	fuel.step({ kFlowRate }, kFrameDt);
	TEST_EXPECT_NEAR(context, fuel.external_fuel(), 0.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, fuel.internal_fuel(), kRemainingInternalFuel, kTolerance);
	const Systems::FuelMassDeltaResult delta = fuel.take_mass_delta();
	TEST_EXPECT(context, delta.available);
	TEST_EXPECT_NEAR(
		context, delta.delta.mass, kExpectedConsumption, kTolerance);
}
}

void run_mass_delta_tests(Tests::Context& context)
{
	test_pending_mass_delta_is_consumed_once(context);
	test_fuel_consumes_registered_demand(context);
	test_external_fuel_is_aggregated_by_station(context);
	test_consumption_crosses_external_fuel_boundary(context);
}
