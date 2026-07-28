#pragma once

#include "TestHarness.h"
#include "../F-CK-1C_EFM/Core/Contracts/Diagnostics.h"
#include "../F-CK-1C_EFM/Core/Simulation/AircraftState.h"
#include "../F-CK-1C_EFM/Core/Systems/SystemPipeline.h"

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace SystemPipelineTest
{
inline constexpr double kNeutralAxis = 0.0;

using SetupAction = std::function<void(Core::Systems::SystemSetup&)>;
using StepAction = std::function<void(
	const Core::Systems::AircraftDataSnapshot&,
	Core::Systems::SystemResult&)>;

struct SystemDefinition
{
	std::string id;
	Core::Systems::SystemGroup group = Core::Systems::SystemGroup::Equipment;
	SetupAction setup;
	StepAction step;
};

class CallbackSystem final : public Core::Systems::System
{
public:
	explicit CallbackSystem(const SystemDefinition& definition)
		: setup_(definition.setup),
		step_(definition.step)
	{
	}

	void setup(Core::Systems::SystemSetup& setup) override
	{
		setup_(setup);
	}

	void step(
		const Core::Systems::AircraftDataSnapshot& snapshot,
		Core::Systems::SystemResult& result) override
	{
		step_(snapshot, result);
	}

private:
	SetupAction setup_;
	StepAction step_;
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
		{}
	};
}

inline Core::Systems::AircraftDataSnapshot step_pipeline(
	Core::Systems::SystemPipeline& pipeline,
	const Core::FrameInput& frame,
	const Core::AircraftObservation& observation)
{
	return pipeline.step({ frame, observation });
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
	catch (const std::logic_error&)
	{
		return true;
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
		TEST_EXPECT(context, actual.owner_type == expected.owner_type);
		TEST_EXPECT(context, actual.owner == expected.owner);
		TEST_EXPECT(context, actual.operation == expected.operation);
		TEST_EXPECT(context, actual.reason == expected.reason);
	}
	TEST_EXPECT(context, caught);
}

inline Core::FlightControlDemand demand(double pitch)
{
	return { pitch, kNeutralAxis, kNeutralAxis };
}

inline Core::PrimaryControlPosition position(double elevator)
{
	return { elevator, kNeutralAxis, kNeutralAxis };
}

inline StepAction no_step()
{
	return [](
		const Core::Systems::AircraftDataSnapshot&,
		Core::Systems::SystemResult&)
	{
	};
}
}
