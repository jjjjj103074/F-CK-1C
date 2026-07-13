#pragma once

#include "FBWControllerTypes.h"
#include "FBWLifecycle.h"

namespace Systems
{
void set_fbw_cat_mode(FBWControllerState& state, FBWCatMode mode);
void toggle_fbw_cat_mode(FBWControllerState& state, bool command_pressed);
void set_fbw_g_limiter_override(FBWControllerState& state, bool enabled);
void toggle_fbw_g_limiter_override(FBWControllerState& state, bool command_pressed);

const char* fbw_mode_name(const FBWControllerState& state);
const char* fbw_state_name(const FBWControllerState& state);
const char* fbw_exit_reason_name(const FBWControllerState& state);

FBWControllerOutput update_fbw_controller(
	FBWControllerState& state,
	const FBWControllerConfig& config,
	const FBWControllerInput& input);
}
