#include "DebugIndicatorCommandHandler.h"

#include "../BridgeContext.h"
#include "../../../DcsIds/CustomCommands.g.h"

#include <cmath>

namespace DcsBridge
{
namespace Internal
{
bool handle_debug_indicator_command(
	BridgeContext& context,
	int command,
	float value)
{
	if (command != DcsIds::Commands::DebugIndicatorToggle)
	{
		return false;
	}
	CockpitParameterEvents events;
	(void)context.perform_flight_action(
		{ "ed_fm_set_command", "command", command },
		[&context, &events, value]()
		{
			if (!std::isfinite(value))
			{
				context.event_reporter().log_invalid_numeric(
					"ed_fm_set_command", "value", value);
				return;
			}
			if (value > 0.0F)
			{
				events = context.debug_indicator_exporter().toggle();
			}
		});
	context.event_reporter().log_cockpit_parameter_events(events);
	return true;
}
}
}
