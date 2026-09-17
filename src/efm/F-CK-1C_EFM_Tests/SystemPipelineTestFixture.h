#pragma once

#include "TestHarness.h"
#include "DebugTelemetryTestSupport.h"
#include "../F-CK-1C_EFM/Core/Diagnostics/ExecutionError.h"
#include "../F-CK-1C_EFM/Core/Simulation/AircraftState.h"
#include "../F-CK-1C_EFM/Core/Systems/SystemPipeline.h"

#include <chrono>
#include <cstdio>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace SystemPipelineTest
{
inline constexpr double kNeutralAxis = 0.0;
inline constexpr std::uint32_t kTestUpdateRateHz = 64;
inline constexpr Core::Systems::SystemScheduledTime kTestUpdatePeriod =
	std::chrono::nanoseconds(15'625'000);

using SetupAction = std::function<void(Core::Systems::SystemSetup&)>;
using StepAction = std::function<void(
	const Core::Systems::AircraftDataView&,
	Core::Systems::SystemResult&)>;
using TimedStepAction = std::function<void(
	const Core::Systems::SystemStepContext&,
	const Core::Systems::AircraftDataView&,
	Core::Systems::SystemResult&)>;

struct SystemDefinition
{
	std::string id;
	Core::Systems::SystemGroup group = Core::Systems::SystemGroup::Equipment;
	SetupAction setup;
	StepAction step;
	std::optional<std::uint32_t> update_rate_hz = kTestUpdateRateHz;
	TimedStepAction timed_step;
};

class CallbackSystem final : public Core::Systems::System
{
public:
	explicit CallbackSystem(const SystemDefinition& definition)
		: setup_(definition.setup),
		step_(definition.step),
		update_rate_hz_(definition.update_rate_hz),
		timed_step_(definition.timed_step)
	{
	}

	void setup(Core::Systems::SystemSetup& setup) override
	{
		if (update_rate_hz_)
		{
			setup.update_rate_hz(*update_rate_hz_);
		}
		setup_(setup);
	}

	void step(
		const Core::Systems::SystemStepContext& context,
		const Core::Systems::AircraftDataView& aircraft,
		Core::Systems::SystemResult& result) override
	{
		if (timed_step_)
		{
			timed_step_(context, aircraft, result);
			return;
		}
		step_(aircraft, result);
	}

private:
	SetupAction setup_;
	StepAction step_;
	std::optional<std::uint32_t> update_rate_hz_;
	TimedStepAction timed_step_;
};

inline Core::Systems::SystemEntry entry(const SystemDefinition& definition)
{
	return {
		definition.id,
		definition.group,
		[definition](const Core::Systems::FlightSetupContext&)
		{
			return std::make_unique<CallbackSystem>(definition);
		}
	};
}

inline Core::Systems::FlightSetupContext flight_setup()
{
	return {
		Core::StartMode::HotGround,
		{},
		{},
		Tests::disabled_debug_telemetry()
	};
}

inline Core::Systems::AircraftDataSnapshot step_pipeline(
	Core::Systems::SystemPipeline& pipeline,
	const Core::FrameInput& frame,
	const Core::AircraftObservation& observation)
{
	return pipeline.step({
		frame,
		observation,
		pipeline.advanced_through() + kTestUpdatePeriod
	});
}

inline Core::Systems::AircraftDataSnapshot step_pipeline_to(
	Core::Systems::SystemPipeline& pipeline,
	const Core::FrameInput& frame,
	const Core::AircraftObservation& observation,
	Core::Systems::SystemScheduledTime target_time)
{
	return pipeline.step({ frame, observation, target_time });
}

inline Core::Systems::AircraftDataSnapshot step_pipeline(
	Core::Systems::SystemPipeline& pipeline)
{
	const Core::FrameInput frame;
	const Core::AircraftObservation observation;
	return step_pipeline(pipeline, frame, observation);
}

inline bool construction_throws(
	const std::vector<Core::Systems::SystemEntry>& catalog)
{
	try
	{
		Core::Systems::SystemPipeline pipeline(flight_setup(), catalog);
	}
	catch (const Core::ExecutionError& error)
	{
		return error.details().operation == "setup";
	}
	return false;
}

template <typename Action>
bool action_throws(const Action& action)
{
	try
	{
		action();
	}
	catch (const std::exception&)
	{
		return true;
	}
	return false;
}

template <typename Action>
void expect_execution_error(
	Tests::Context& context,
	const Action& action,
	const Core::ExecutionErrorDetails& expected)
{
	bool caught = false;
	try
	{
		action();
	}
	catch (const Core::ExecutionError& error)
	{
		caught = true;
		const Core::ExecutionErrorDetails& actual = error.details();
		if (actual.owner_type != expected.owner_type ||
			actual.owner != expected.owner ||
			actual.operation != expected.operation ||
			actual.reason != expected.reason)
		{
			std::printf(
				"ExecutionError expected [%s/%s/%s], actual [%s/%s/%s]\n",
				expected.owner.c_str(), expected.operation.c_str(),
				expected.reason.c_str(), actual.owner.c_str(),
				actual.operation.c_str(), actual.reason.c_str());
		}
		TEST_EXPECT(context, actual.owner_type == expected.owner_type);
		TEST_EXPECT(context, actual.owner == expected.owner);
		TEST_EXPECT(context, actual.operation == expected.operation);
		TEST_EXPECT(context, actual.reason == expected.reason);
	}
	TEST_EXPECT(context, caught);
}

inline Core::FlightControlActuatorCommand actuator_command(
	double symmetric_stabilator_rad)
{
	return { symmetric_stabilator_rad, kNeutralAxis, kNeutralAxis };
}

inline Core::FlightControlActuatorState actuator_state(
	double symmetric_stabilator_rad)
{
	Core::FlightControlActuatorState state;
	state.symmetric_stabilator.position_rad = symmetric_stabilator_rad;
	return state;
}

inline StepAction no_step()
{
	return [](
		const Core::Systems::AircraftDataView&,
		Core::Systems::SystemResult&)
	{
	};
}
}
