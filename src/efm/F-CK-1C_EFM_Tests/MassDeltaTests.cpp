#include "TestHarness.h"

#include "Core/Simulation/Models/MassProperties/MassPropertiesModel.h"
#include "Core/Systems/Fuel/Fuel.h"

namespace
{
constexpr double kTolerance = 1e-9;
constexpr double kPendingConsumption = 12.5;
constexpr double kFlowRate = 2.5;
constexpr double kFrameDt = 0.2;
constexpr double kExpectedConsumption = 0.5;
constexpr double kExpectedTwoTickConsumption = 1.0;
constexpr double kExternalFuel = 10.0;
constexpr double kInternalFuel = 10.0;
constexpr double kConsumedInternalFuel = 9.5;
constexpr double kTwoTickRemainingInternalFuel = 9.0;
constexpr double kBoundaryExternalFuel = 0.25;
constexpr double kRemainingInternalFuel = 9.75;
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

void test_mass_properties_projects_consumed_fuel(Tests::Context& context)
{
	Core::Simulation::MassPropertiesModel model;
	Core::FuelData fuel;
	fuel.consumed_mass_kg = kPendingConsumption;
	const Core::MassDeltaResult& result = model.step(fuel);
	TEST_EXPECT(context, result.available);
	TEST_EXPECT_NEAR(
		context, result.delta.mass_kg, kPendingConsumption, kTolerance);
	TEST_EXPECT_NEAR(
		context, result.delta.position_body_m.x, kMassPositionX, kTolerance);
	TEST_EXPECT_NEAR(
		context, result.delta.position_body_m.y, kMassPositionY, kTolerance);
	TEST_EXPECT_NEAR(
		context, result.delta.position_body_m.z, kMassPositionZ, kTolerance);
	TEST_EXPECT_NEAR(
		context, result.delta.moment_of_inertia_delta_kg_m2.x,
		0.0, kTolerance);
}

void test_mass_properties_ignores_zero_consumption(Tests::Context& context)
{
	Core::Simulation::MassPropertiesModel model;
	TEST_EXPECT(context, !model.step({}).available);
}

void test_fuel_consumes_registered_demand(Tests::Context& context)
{
	Core::Systems::Fuel fuel;
	fuel.set_external_fuel({ kFirstFuelStation, kExternalFuel, {} });
	fuel.begin_frame(false);
	const Core::FuelData& data =
		fuel.step({ kFlowRate }, kFrameDt);
	TEST_EXPECT_NEAR(
		context, data.total_fuel_flow_kg_s, kFlowRate, kTolerance);
	TEST_EXPECT_NEAR(
		context,
		data.consumed_mass_kg,
		kExpectedConsumption,
		kTolerance);
}

void test_fuel_suppression_is_one_frame_and_preserves_flow(
	Tests::Context& context)
{
	Core::Systems::Fuel fuel;
	fuel.set_internal_fuel(kInternalFuel);
	fuel.begin_frame(true);
	const Core::FuelData suppressed =
		fuel.step({ kFlowRate }, kFrameDt);
	TEST_EXPECT_NEAR(
		context, fuel.internal_fuel_kg(), kInternalFuel, kTolerance);
	TEST_EXPECT_NEAR(
		context, suppressed.total_fuel_flow_kg_s, kFlowRate, kTolerance);
	TEST_EXPECT_NEAR(context, suppressed.consumed_mass_kg, 0.0, kTolerance);
	fuel.begin_frame(false);
	const Core::FuelData resumed = fuel.step({ kFlowRate }, kFrameDt);
	TEST_EXPECT_NEAR(
		context, fuel.internal_fuel_kg(), kConsumedInternalFuel, kTolerance);
	TEST_EXPECT_NEAR(
		context, resumed.consumed_mass_kg, kExpectedConsumption, kTolerance);
}

void test_fuel_accumulates_multiple_ticks_within_one_frame(
	Tests::Context& context)
{
	Core::Systems::Fuel fuel;
	fuel.set_internal_fuel(kInternalFuel);
	fuel.begin_frame(false);
	(void)fuel.step({ kFlowRate }, kFrameDt);
	const Core::FuelData& second = fuel.step({ kFlowRate }, kFrameDt);
	TEST_EXPECT_NEAR(
		context,
		second.consumed_mass_kg,
		kExpectedTwoTickConsumption,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		fuel.internal_fuel_kg(),
		kTwoTickRemainingInternalFuel,
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
		context, fuel.external_fuel_kg(), kTotalStationFuel, kTolerance);
	fuel.set_external_fuel({
		kFirstFuelStation,
		kUpdatedFirstStationFuel,
		{ kMassPositionY, kMassPositionZ, kMassPositionZ }
	});
	TEST_EXPECT_NEAR(
		context, fuel.external_fuel_kg(), kUpdatedTotalStationFuel, kTolerance);
	fuel.set_external_fuel({ kSecondFuelStation, kMassPositionZ, {} });
	TEST_EXPECT_NEAR(
		context, fuel.external_fuel_kg(), kUpdatedFirstStationFuel, kTolerance);
}

void test_consumption_crosses_external_fuel_boundary(Tests::Context& context)
{
	Core::Systems::Fuel fuel;
	fuel.set_internal_fuel(kInternalFuel);
	fuel.set_external_fuel({
		kFirstFuelStation, kBoundaryExternalFuel, {}
	});
	fuel.begin_frame(false);
	fuel.step({ kFlowRate }, kFrameDt);
	TEST_EXPECT_NEAR(context, fuel.external_fuel_kg(), 0.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, fuel.internal_fuel_kg(), kRemainingInternalFuel, kTolerance);
	TEST_EXPECT_NEAR(
		context, fuel.data().consumed_mass_kg,
		kExpectedConsumption, kTolerance);
}
}

void run_mass_delta_tests(Tests::Context& context)
{
	test_mass_properties_projects_consumed_fuel(context);
	test_mass_properties_ignores_zero_consumption(context);
	test_fuel_consumes_registered_demand(context);
	test_fuel_suppression_is_one_frame_and_preserves_flow(context);
	test_fuel_accumulates_multiple_ticks_within_one_frame(context);
	test_external_fuel_is_aggregated_by_station(context);
	test_consumption_crosses_external_fuel_boundary(context);
}
