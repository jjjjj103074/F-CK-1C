#include "Fck1cEfmTestFixture.h"
#include "SystemPipelineTestFixture.h"

#include <algorithm>
#include <vector>

namespace
{
using namespace Core;
using namespace Core::Systems;

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
		CommandGroup::PitchRoll, CommandId::SetPitchAxis, kPitchCommand });
	(void)pipeline.send({
		CommandGroup::Yaw, CommandId::SetYawAxis, kYawCommand });
	(void)pipeline.send({
		CommandGroup::Throttle,
		CommandId::SetCommonThrottleAxis,
		kThrottleCommand
	});
	(void)pipeline.send({
		CommandGroup::LandingGear, CommandId::SetGear, kGearUpCommand });
	(void)pipeline.send({
		CommandGroup::Airframe, CommandId::SetFlapsAuto, kActiveCommand });
}

void test_production_output_ignores_same_group_entry_order(
	Tests::Context& context)
{
	auto forward_catalog = load_generated_system_catalog();
	auto reversed_catalog = load_generated_system_catalog();
	std::reverse(reversed_catalog.begin(), reversed_catalog.end());
	const FlightSetupContext setup = {
		Data::fck1c_aircraft_config(),
		StartMode::HotGround,
		{ kInitialFuel, {} }
	};
	SystemPipeline forward(setup, std::move(forward_catalog));
	SystemPipeline reversed(setup, std::move(reversed_catalog));
	send_scenario_commands(forward);
	send_scenario_commands(reversed);
	const FrameInput input = Tests::Fck1c::make_frame_input();
	for (std::size_t frame = 0; frame < kComparisonFrameCount; ++frame)
	{
		TEST_EXPECT(
			context,
			published_values(forward.step(input)) ==
				published_values(reversed.step(input)));
	}
}

void test_equipment_reads_current_normalized_observation(
	Tests::Context& context)
{
	SystemPipeline pipeline(SystemPipelineTest::flight_setup());
	(void)pipeline.send({
		CommandGroup::Yaw,
		CommandId::SetYawAxis,
		kYawCommand
	});
	FrameInput input;
	input.dt_s = kFrameDt;
	input.availability.world_kinematics = true;
	input.world_kinematics.velocity.x = kHighSpeedObservation;
	const LandingGearData& gear = pipeline.step(input).read(
		AircraftDataKeys::kLandingGearData);
	TEST_EXPECT_NEAR(
		context,
		gear.nose_wheel_steering,
		kNeutralSteering,
		kTolerance);
}
}

void run_phase_four_system_timing_tests(Tests::Context& context)
{
	test_production_output_ignores_same_group_entry_order(context);
	test_equipment_reads_current_normalized_observation(context);
}
