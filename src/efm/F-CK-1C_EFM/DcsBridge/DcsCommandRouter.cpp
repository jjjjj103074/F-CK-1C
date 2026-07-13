#include "DcsCommandRouter.h"

#include "../DcsIds/Commands.h"

namespace
{
constexpr double kPitchTrimStep = 0.0015;
constexpr double kRollTrimStep = 0.001;
constexpr double kYawTrimStep = 0.001;
constexpr double kThrottleStep = 0.0075;
constexpr float kCommandPressedThreshold = 0.5f;

bool command_pressed(float value)
{
	return value > kCommandPressedThreshold;
}

bool route_pitch_command(Systems::PrimaryControlState& controls, int command, float value)
{
	using namespace DcsIds::Commands;
	switch (command)
	{
	case JoystickPitch: Systems::set_pitch_axis_input(controls, value); return true;
	case PitchUp: Systems::set_pitch_discrete_input(controls, 1); return true;
	case PitchUpStop: Systems::set_pitch_discrete_input(controls, 0); return true;
	case PitchDown: Systems::set_pitch_discrete_input(controls, -1); return true;
	case PitchDownStop: Systems::set_pitch_discrete_input(controls, 0); return true;
	case TrimUp: Systems::adjust_pitch_trim(controls, kPitchTrimStep); return true;
	case TrimDown: Systems::adjust_pitch_trim(controls, -kPitchTrimStep); return true;
	default: return false;
	}
}

bool route_roll_command(Systems::PrimaryControlState& controls, int command, float value)
{
	using namespace DcsIds::Commands;
	switch (command)
	{
	case JoystickRoll: Systems::set_roll_axis_input(controls, value); return true;
	case RollLeft: Systems::set_roll_discrete_input(controls, -1); return true;
	case RollLeftStop: Systems::set_roll_discrete_input(controls, 0); return true;
	case RollRight: Systems::set_roll_discrete_input(controls, 1); return true;
	case RollRightStop: Systems::set_roll_discrete_input(controls, 0); return true;
	case TrimLeft: Systems::adjust_roll_trim(controls, -kRollTrimStep); return true;
	case TrimRight: Systems::adjust_roll_trim(controls, kRollTrimStep); return true;
	default: return false;
	}
}

bool route_yaw_command(Systems::PrimaryControlState& controls, int command, float value)
{
	using namespace DcsIds::Commands;
	switch (command)
	{
	case PedalYaw: Systems::set_yaw_axis_input(controls, value); return true;
	case RudderLeft: Systems::set_yaw_discrete_input(controls, 1); return true;
	case RudderLeftStop: Systems::set_yaw_discrete_input(controls, 0); return true;
	case RudderRight: Systems::set_yaw_discrete_input(controls, -1); return true;
	case RudderRightStop: Systems::set_yaw_discrete_input(controls, 0); return true;
	case RudderTrimLeft: Systems::adjust_yaw_trim(controls, kYawTrimStep); return true;
	case RudderTrimRight: Systems::adjust_yaw_trim(controls, -kYawTrimStep); return true;
	case ResetTrim: Systems::reset_primary_trims(controls); return true;
	default: return false;
	}
}

bool route_fbw_command(Systems::FBWControllerState& fbw, int command, float value)
{
	using namespace DcsIds::Commands;
	switch (command)
	{
	case FBWCatToggle: Systems::toggle_fbw_cat_mode(fbw, command_pressed(value)); return true;
	case FBWCat1:
		if (command_pressed(value)) Systems::set_fbw_cat_mode(fbw, Systems::FBW_CAT1);
		return true;
	case FBWCat3:
		if (command_pressed(value)) Systems::set_fbw_cat_mode(fbw, Systems::FBW_CAT3);
		return true;
	case FBWGLimiterOverride:
		Systems::set_fbw_g_limiter_override(fbw, command_pressed(value)); return true;
	case FBWGLimiterOverrideToggle:
		Systems::toggle_fbw_g_limiter_override(fbw, command_pressed(value)); return true;
	default: return false;
	}
}

bool route_engine_command(Systems::EngineSystemState& engines, int command)
{
	using namespace DcsIds::Commands;
	switch (command)
	{
	case EnginesOn: Systems::set_both_engine_switches(engines, true); return true;
	case LeftEngineOn: Systems::set_left_engine_switch(engines, true); return true;
	case RightEngineOn: Systems::set_right_engine_switch(engines, true); return true;
	case EnginesOff: Systems::set_both_engine_switches(engines, false); return true;
	case LeftEngineOff: Systems::set_left_engine_switch(engines, false); return true;
	case RightEngineOff: Systems::set_right_engine_switch(engines, false); return true;
	default: return false;
	}
}

bool route_throttle_axis_command(
	Systems::ThrottleInputState& throttles,
	int command,
	float value)
{
	using namespace DcsIds::Commands;
	switch (command)
	{
	case ThrottleAxis: Systems::set_common_throttle_axis(throttles, value); return true;
	case ThrottleAxisLeft: Systems::set_left_throttle_axis(throttles, value); return true;
	case ThrottleAxisRight: Systems::set_right_throttle_axis(throttles, value); return true;
	default: return false;
	}
}

bool route_throttle_keyboard_command(Systems::ThrottleInputState& throttles, int command)
{
	using namespace DcsIds::Commands;
	switch (command)
	{
	case ThrottleIncrease: Systems::step_common_keyboard_throttle(throttles, kThrottleStep); return true;
	case ThrottleLeftUp: Systems::step_left_keyboard_throttle(throttles, kThrottleStep); return true;
	case ThrottleRightUp: Systems::step_right_keyboard_throttle(throttles, kThrottleStep); return true;
	case ThrottleDecrease: Systems::step_common_keyboard_throttle(throttles, -kThrottleStep); return true;
	case ThrottleLeftDown: Systems::step_left_keyboard_throttle(throttles, -kThrottleStep); return true;
	case ThrottleRightDown: Systems::step_right_keyboard_throttle(throttles, -kThrottleStep); return true;
	case ThrottleStop: return true;
	default: return false;
	}
}

bool route_throttle_command(Systems::ThrottleInputState& throttles, int command, float value)
{
	return route_throttle_axis_command(throttles, command, value) ||
		route_throttle_keyboard_command(throttles, command);
}

bool route_airbrake_command(Systems::AirframeDeviceState& devices, int command)
{
	using namespace DcsIds::Commands;
	switch (command)
	{
	case AirBrakes: Systems::toggle_airbrake(devices); return true;
	case AirBrakesOff: Systems::set_airbrake(devices, false); return true;
	case AirBrakesOn: Systems::set_airbrake(devices, true); return true;
	case AirBrakesAuto: return true;
	case AirBrakesUp: Systems::set_airbrake(devices, false); return true;
	case AirBrakesDown: Systems::set_airbrake(devices, true); return true;
	default: return false;
	}
}

bool route_flap_command(Systems::AirframeDeviceState& devices, int command)
{
	using namespace DcsIds::Commands;
	switch (command)
	{
	case FlapsToggle: Systems::toggle_flap_mode(devices); return true;
	case FlapsDown: Systems::set_flap_mode(devices, Systems::FLAP_MODE_DOWN); return true;
	case FlapsUp: Systems::set_flap_mode(devices, Systems::FLAP_MODE_UP); return true;
	case FlapsAuto: Systems::set_flap_mode(devices, Systems::FLAP_MODE_AUTO); return true;
	case FlapsUpCmd: Systems::set_flap_mode(devices, Systems::FLAP_MODE_UP); return true;
	case FlapsDownCmd: Systems::set_flap_mode(devices, Systems::FLAP_MODE_DOWN); return true;
	default: return false;
	}
}

bool route_gear_command(Systems::LandingGearSystemState& gear, int command)
{
	using namespace DcsIds::Commands;
	switch (command)
	{
	case GearToggle: Systems::toggle_gear(gear); return true;
	case GearDown: Systems::set_gear(gear, true); return true;
	case GearUp: Systems::set_gear(gear, false); return true;
	case GearAuto: return true;
	case GearHandleUp: Systems::set_gear(gear, false); return true;
	case GearHandleDown: Systems::set_gear(gear, true); return true;
	default: return false;
	}
}

bool route_nose_wheel_command(Systems::WheelState& wheels, int command, float value)
{
	using namespace DcsIds::Commands;
	switch (command)
	{
	case NoseTurnToggle: Systems::toggle_nose_turn_enabled(wheels, command_pressed(value)); return true;
	case NoseTurnUp: Systems::set_nose_turn_enabled(wheels, false); return true;
	case NoseTurnAuto: Systems::set_nose_turn_enabled(wheels, false); return true;
	case NoseTurnDown: Systems::set_nose_turn_enabled(wheels, true); return true;
	default: return false;
	}
}

bool route_brake_command(Systems::WheelState& wheels, int command, float value)
{
	using namespace DcsIds::Commands;
	switch (command)
	{
	case WheelBrakeAxis: Systems::set_brake_axis(wheels, Systems::normalize_brake_axis(value)); return true;
	case WheelBrakeAxisLeft: Systems::set_left_brake(wheels, Systems::normalize_brake_axis(value)); return true;
	case WheelBrakeAxisRight: Systems::set_right_brake(wheels, Systems::normalize_brake_axis(value)); return true;
	case WheelBrakeOn: Systems::set_brake_axis(wheels, 1.0); return true;
	case WheelBrakeOff: Systems::set_brake_axis(wheels, 0.0); return true;
	case WheelBrakeLeftOn: Systems::set_left_brake(wheels, 1.0); return true;
	case WheelBrakeLeftOff: Systems::set_left_brake(wheels, 0.0); return true;
	case WheelBrakeRightOn: Systems::set_right_brake(wheels, 1.0); return true;
	case WheelBrakeRightOff: Systems::set_right_brake(wheels, 0.0); return true;
	default: return false;
	}
}

bool route_flight_control_command(Core::Fck1cEfmSystems& systems, int command, float value)
{
	return route_pitch_command(systems.primary_controls, command, value) ||
		route_roll_command(systems.primary_controls, command, value) ||
		route_yaw_command(systems.primary_controls, command, value) ||
		route_fbw_command(systems.fbw, command, value);
}

void route_aircraft_command(Core::Fck1cEfmSystems& systems, int command, float value)
{
	if (route_engine_command(systems.engines, command) ||
		route_throttle_command(systems.throttle_inputs, command, value) ||
		route_airbrake_command(systems.airframe_devices, command) ||
		route_flap_command(systems.airframe_devices, command) ||
		route_gear_command(systems.landing_gear, command) ||
		route_nose_wheel_command(systems.landing_gear.wheels, command, value))
	{
		return;
	}
	route_brake_command(systems.landing_gear.wheels, command, value);
}
}

namespace DcsBridge
{
void route_command(Core::Fck1cEfmSystems& systems, int command, float value)
{
	if (!route_flight_control_command(systems, command, value))
	{
		route_aircraft_command(systems, command, value);
	}
}
}
