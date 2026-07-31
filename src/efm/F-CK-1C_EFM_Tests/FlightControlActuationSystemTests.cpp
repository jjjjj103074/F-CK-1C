#include "SystemPipelineTestFixture.h"

#include "Common/Units.h"
#include "Core/Systems/FlightControlActuationSystem/FlightControlActuationSystem.h"
#include "Core/Systems/SystemUpdateRates.h"

#include <chrono>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace
{
using namespace Core;
using namespace Core::Systems;
using namespace SystemPipelineTest;

constexpr double kTolerance = 1.0e-9;
constexpr double kFullCommand = 1.0;
constexpr double kOvertravelCommand = 2.0;
constexpr double kActuatorDt = 1.0 / 256.0;
constexpr SystemScheduledTime kActuatorPeriod =
	std::chrono::nanoseconds(3'906'250);

static_assert(
	kFlightControlActuationProjectUpdateRateHz == 256,
	"The project-defined actuation integration rate must remain 256 Hz.");
static_assert(
	std::is_same<
		decltype(fck1c_flight_control_actuation_system_config()),
		const FlightControlActuationSystemConfig&>::value,
	"Production actuation configuration must be read-only.");

bool rejects_invalid_config(FlightControlActuationSystemConfig config)
{
	try
	{
		(void)make_flight_control_actuation_system_entry(config);
	}
	catch (const std::invalid_argument&)
	{
		return true;
	}
	return false;
}

void test_production_config_uses_radians(Tests::Context& context)
{
	const auto& config = fck1c_flight_control_actuation_system_config();
	TEST_EXPECT_NEAR(
		context, config.elevator.travel_limit_rad,
		Common::rad(25.0), kTolerance);
	TEST_EXPECT_NEAR(
		context, config.aileron.rate_limit_rad_s,
		Common::rad(110.0), kTolerance);
	TEST_EXPECT_NEAR(
		context, config.rudder.travel_limit_rad,
		Common::rad(30.0), kTolerance);
}

void test_invalid_config_is_rejected(Tests::Context& context)
{
	auto config = fck1c_flight_control_actuation_system_config();
	config.elevator.rate_limit_rad_s = 0.0;
	TEST_EXPECT(context, rejects_invalid_config(config));
	config = fck1c_flight_control_actuation_system_config();
	config.rudder.lag_time_constant_s =
		std::numeric_limits<double>::quiet_NaN();
	TEST_EXPECT(context, rejects_invalid_config(config));
}

void test_actuator_owns_physical_dynamics(Tests::Context& context)
{
	FlightControlActuationSystem system(
		fck1c_flight_control_actuation_system_config());
	const FlightControlActuatorState& first = system.update(
		{ kFullCommand, 0.0, 0.0 }, kActuatorDt);
	TEST_EXPECT(context, first.elevator.position_rad > 0.0);
	TEST_EXPECT(context, first.elevator.normalized_position < kFullCommand);
	TEST_EXPECT(context, first.elevator.rate_rad_s > 0.0);
	TEST_EXPECT(context, first.elevator.saturated);
	const FlightControlActuatorState& limited = system.update(
		{ kOvertravelCommand, 0.0, 0.0 }, kActuatorDt);
	TEST_EXPECT(context, limited.elevator.saturated);
	TEST_EXPECT(context, limited.any_saturated);
}

SystemEntry command_source()
{
	const SystemDefinition source = {
		"actuator_command_source",
		SystemGroup::Control,
		[](SystemSetup& setup)
		{
			setup.publish(
				AircraftDataKeys::kFlightControlActuatorCommand,
				actuator_command(kFullCommand));
		},
		no_step()
	};
	return entry(source);
}

void test_pipeline_runs_actuator_at_256_hz(Tests::Context& context)
{
	SystemPipeline pipeline(
		flight_setup(),
		{
			command_source(),
			make_flight_control_actuation_system_entry(
				fck1c_flight_control_actuation_system_config())
		});
	const FrameInput frame;
	const AircraftObservation observation;
	const auto before = step_pipeline_to(
		pipeline, frame, observation,
		kActuatorPeriod - std::chrono::nanoseconds(1));
	TEST_EXPECT_NEAR(
		context,
		before.read(AircraftDataKeys::kFlightControlActuatorState)
			.elevator.position_rad,
		0.0,
		kTolerance);
	const auto after = step_pipeline_to(
		pipeline, frame, observation, kActuatorPeriod);
	TEST_EXPECT(
		context,
		after.read(AircraftDataKeys::kFlightControlActuatorState)
			.elevator.position_rad > 0.0);
}
}

void run_flight_control_actuation_system_tests(Tests::Context& context)
{
	test_production_config_uses_radians(context);
	test_invalid_config_is_rejected(context);
	test_actuator_owns_physical_dynamics(context);
	test_pipeline_runs_actuator_at_256_hz(context);
}
