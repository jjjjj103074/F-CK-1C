#pragma once

namespace DcsBridge
{
namespace Internal
{
class BridgeContext;

bool handle_debug_indicator_command(
	BridgeContext& context,
	int command,
	float value);
}
}
