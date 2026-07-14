#include "DcsCommandRouter.h"

#include "../DcsIds/Commands.h"

#include <cstddef>

namespace
{
constexpr double kPitchTrimStep = 0.0015;
constexpr double kRollTrimStep = 0.001;
constexpr double kYawTrimStep = 0.001;
constexpr double kThrottleStep = 0.0075;
constexpr float kCommandPressedThreshold = 0.5f;

enum class ValueSource
{
	Input,
	Constant,
	Pressed
};

struct CommandBinding
{
	int dcs_id;
	Core::CommandGroup group;
	Core::CommandAction action;
	ValueSource value_source;
	double constant;
};

#define BIND_INPUT(id, group, action) \
	{ DcsIds::Commands::id, Core::CommandGroup::group, Core::CommandAction::action, ValueSource::Input, 0.0 }
#define BIND_CONST(id, group, action, value) \
	{ DcsIds::Commands::id, Core::CommandGroup::group, Core::CommandAction::action, ValueSource::Constant, value }
#define BIND_PRESS(id, group, action) \
	{ DcsIds::Commands::id, Core::CommandGroup::group, Core::CommandAction::action, ValueSource::Pressed, 0.0 }

constexpr CommandBinding kBindings[] = {
	BIND_INPUT(JoystickPitch, PitchRoll, SetPitchAxis),
	BIND_CONST(PitchUp, PitchRoll, SetPitchDiscrete, 1.0),
	BIND_CONST(PitchUpStop, PitchRoll, SetPitchDiscrete, 0.0),
	BIND_CONST(PitchDown, PitchRoll, SetPitchDiscrete, -1.0),
	BIND_CONST(PitchDownStop, PitchRoll, SetPitchDiscrete, 0.0),
	BIND_CONST(TrimUp, PitchRoll, AdjustPitchTrim, kPitchTrimStep),
	BIND_CONST(TrimDown, PitchRoll, AdjustPitchTrim, -kPitchTrimStep),
	BIND_INPUT(JoystickRoll, PitchRoll, SetRollAxis),
	BIND_CONST(RollLeft, PitchRoll, SetRollDiscrete, -1.0),
	BIND_CONST(RollLeftStop, PitchRoll, SetRollDiscrete, 0.0),
	BIND_CONST(RollRight, PitchRoll, SetRollDiscrete, 1.0),
	BIND_CONST(RollRightStop, PitchRoll, SetRollDiscrete, 0.0),
	BIND_CONST(TrimLeft, PitchRoll, AdjustRollTrim, -kRollTrimStep),
	BIND_CONST(TrimRight, PitchRoll, AdjustRollTrim, kRollTrimStep),
	BIND_INPUT(PedalYaw, Yaw, SetYawAxis),
	BIND_CONST(RudderLeft, Yaw, SetYawDiscrete, 1.0),
	BIND_CONST(RudderLeftStop, Yaw, SetYawDiscrete, 0.0),
	BIND_CONST(RudderRight, Yaw, SetYawDiscrete, -1.0),
	BIND_CONST(RudderRightStop, Yaw, SetYawDiscrete, 0.0),
	BIND_CONST(RudderTrimLeft, Yaw, AdjustYawTrim, kYawTrimStep),
	BIND_CONST(RudderTrimRight, Yaw, AdjustYawTrim, -kYawTrimStep),
	BIND_CONST(ResetTrim, Yaw, ResetTrim, 0.0),
	BIND_PRESS(FBWCatToggle, Fbw, ToggleFbwCat),
	BIND_PRESS(FBWCat1, Fbw, SetFbwCat1),
	BIND_PRESS(FBWCat3, Fbw, SetFbwCat3),
	BIND_PRESS(FBWGLimiterOverride, Fbw, SetGLimiterOverride),
	BIND_PRESS(FBWGLimiterOverrideToggle, Fbw, ToggleGLimiterOverride),
	BIND_CONST(EnginesOn, Engine, SetBothEngines, 1.0),
	BIND_CONST(LeftEngineOn, Engine, SetLeftEngine, 1.0),
	BIND_CONST(RightEngineOn, Engine, SetRightEngine, 1.0),
	BIND_CONST(EnginesOff, Engine, SetBothEngines, 0.0),
	BIND_CONST(LeftEngineOff, Engine, SetLeftEngine, 0.0),
	BIND_CONST(RightEngineOff, Engine, SetRightEngine, 0.0),
	BIND_INPUT(ThrottleAxis, Throttle, SetCommonThrottleAxis),
	BIND_INPUT(ThrottleAxisLeft, Throttle, SetLeftThrottleAxis),
	BIND_INPUT(ThrottleAxisRight, Throttle, SetRightThrottleAxis),
	BIND_CONST(ThrottleIncrease, Throttle, StepCommonThrottle, kThrottleStep),
	BIND_CONST(ThrottleLeftUp, Throttle, StepLeftThrottle, kThrottleStep),
	BIND_CONST(ThrottleRightUp, Throttle, StepRightThrottle, kThrottleStep),
	BIND_CONST(ThrottleDecrease, Throttle, StepCommonThrottle, -kThrottleStep),
	BIND_CONST(ThrottleLeftDown, Throttle, StepLeftThrottle, -kThrottleStep),
	BIND_CONST(ThrottleRightDown, Throttle, StepRightThrottle, -kThrottleStep),
	BIND_CONST(ThrottleStop, Throttle, NoOp, 0.0),
	BIND_CONST(AirBrakes, Airframe, ToggleAirbrake, 0.0),
	BIND_CONST(AirBrakesOff, Airframe, SetAirbrake, 0.0),
	BIND_CONST(AirBrakesOn, Airframe, SetAirbrake, 1.0),
	BIND_CONST(AirBrakesAuto, Airframe, NoOp, 0.0),
	BIND_CONST(AirBrakesUp, Airframe, SetAirbrake, 0.0),
	BIND_CONST(AirBrakesDown, Airframe, SetAirbrake, 1.0),
	BIND_CONST(FlapsToggle, Airframe, ToggleFlaps, 0.0),
	BIND_CONST(FlapsDown, Airframe, SetFlapsDown, 0.0),
	BIND_CONST(FlapsUp, Airframe, SetFlapsUp, 0.0),
	BIND_CONST(FlapsAuto, Airframe, SetFlapsAuto, 0.0),
	BIND_CONST(FlapsUpCmd, Airframe, SetFlapsUp, 0.0),
	BIND_CONST(FlapsDownCmd, Airframe, SetFlapsDown, 0.0),
	BIND_CONST(GearToggle, LandingGear, ToggleGear, 0.0),
	BIND_CONST(GearDown, LandingGear, SetGear, 1.0),
	BIND_CONST(GearUp, LandingGear, SetGear, 0.0),
	BIND_CONST(GearAuto, LandingGear, NoOp, 0.0),
	BIND_CONST(GearHandleUp, LandingGear, SetGear, 0.0),
	BIND_CONST(GearHandleDown, LandingGear, SetGear, 1.0),
	BIND_PRESS(NoseTurnToggle, LandingGear, ToggleNoseWheelSteering),
	BIND_CONST(NoseTurnUp, LandingGear, SetNoseWheelSteering, 0.0),
	BIND_CONST(NoseTurnAuto, LandingGear, SetNoseWheelSteering, 0.0),
	BIND_CONST(NoseTurnDown, LandingGear, SetNoseWheelSteering, 1.0),
	BIND_INPUT(WheelBrakeAxis, LandingGear, SetBrake),
	BIND_INPUT(WheelBrakeAxisLeft, LandingGear, SetLeftBrake),
	BIND_INPUT(WheelBrakeAxisRight, LandingGear, SetRightBrake),
	BIND_CONST(WheelBrakeOn, LandingGear, SetBrake, 1.0),
	BIND_CONST(WheelBrakeOff, LandingGear, SetBrake, 0.0),
	BIND_CONST(WheelBrakeLeftOn, LandingGear, SetLeftBrake, 1.0),
	BIND_CONST(WheelBrakeLeftOff, LandingGear, SetLeftBrake, 0.0),
	BIND_CONST(WheelBrakeRightOn, LandingGear, SetRightBrake, 1.0),
	BIND_CONST(WheelBrakeRightOff, LandingGear, SetRightBrake, 0.0)
};

#undef BIND_INPUT
#undef BIND_CONST
#undef BIND_PRESS

double mapped_value(const CommandBinding& binding, float input)
{
	switch (binding.value_source)
	{
	case ValueSource::Input: return input;
	case ValueSource::Pressed: return input > kCommandPressedThreshold ? 1.0 : 0.0;
	case ValueSource::Constant: return binding.constant;
	}
	return 0.0;
}
}

namespace DcsBridge
{
DcsCommandMapping map_command(int command, float value)
{
	for (std::size_t index = 0; index < sizeof(kBindings) / sizeof(kBindings[0]); ++index)
	{
		const CommandBinding& binding = kBindings[index];
		if (binding.dcs_id == command)
		{
			return {
				true,
				{ binding.group, binding.action, mapped_value(binding, value) }
			};
		}
	}
	return {};
}
}
