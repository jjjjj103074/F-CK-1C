#include "PropulsionDiagnostics.h"

#include "../SystemPipeline.h"

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
			[this](const Command& command) { handle_command(command); });
	}
}

void PropulsionDiagnostics::step(
	const AircraftDataView&,
	SystemResult& result)
{
	result.publish(AircraftDataKeys::kPropulsionTestIntent, intent_);
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
