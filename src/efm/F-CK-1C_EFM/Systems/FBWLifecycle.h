#pragma once

#include "FBWController.h"

namespace Systems
{
inline void reset_fbw_throttle_interface(FBWControllerState& state)
{
	state.throttle_cmd_left = 0.0;
	state.throttle_cmd_right = 0.0;
	state.throttle_blend = 0.0;
	state.throttle_override = false;
}
}
