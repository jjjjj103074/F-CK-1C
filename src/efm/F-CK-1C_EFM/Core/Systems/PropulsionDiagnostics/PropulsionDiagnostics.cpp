#include "PropulsionDiagnostics.h"

#include "../SystemPipeline.h"
#include "../SystemUpdateRates.h"

namespace
{
constexpr double kEnabledCommandThreshold = 0.5;
}

namespace Core
{
namespace Systems
{
void PropulsionDiagnostics::setup(SystemSetup& setup)
{
	thrust_cut_debug_ = setup.declare_debug_channel<bool>({
		"propulsion_test_thrust_cut_requested",
		"Propulsion Test Thrust Cut",
		DebugTelemetryValueType::Boolean,
		"",
		"Developer-only propulsion test intent."
	});
	setup.update_rate_hz(kProjectDefinedFallbackUpdateRateHz);
	setup.publish(AircraftDataKeys::kPropulsionTestIntent, intent_);
	const CommandId commands[] = {
		CommandId::ToggleThrustCutTest,
		CommandId::EnableThrustCutTest,
		CommandId::DisableThrustCutTest
	};
	for (CommandId id : commands)
	{
		setup.register_command_handler(
			id,
			[this](const SystemActionContext& context, const Command& command)
			{
				handle_command(command);
				thrust_cut_debug_.publish(
					context.simulation_time,
					intent_.thrust_cut_requested);
			});
	}
	thrust_cut_debug_.publish({}, intent_.thrust_cut_requested);
}

void PropulsionDiagnostics::step(
	const SystemStepContext& context,
	const AircraftDataView&,
	SystemResult& result)
{
	result.publish(AircraftDataKeys::kPropulsionTestIntent, intent_);
	thrust_cut_debug_.publish(
		context.scheduled_time,
		intent_.thrust_cut_requested);
}

void PropulsionDiagnostics::handle_command(const Command& command)
{
	if (command.value <= kEnabledCommandThreshold)
	{
		return;
	}
	switch (command.id)
	{
	case CommandId::ToggleThrustCutTest:
		intent_.thrust_cut_requested = !intent_.thrust_cut_requested;
		break;
	case CommandId::EnableThrustCutTest:
		intent_.thrust_cut_requested = true;
		break;
	case CommandId::DisableThrustCutTest:
		intent_.thrust_cut_requested = false;
		break;
	default:
		break;
	}
}

const PropulsionTestIntent& PropulsionDiagnostics::intent() const
{
	return intent_;
}
}
}
