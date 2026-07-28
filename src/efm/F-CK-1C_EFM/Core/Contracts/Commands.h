#pragma once

namespace Core
{
enum class CommandId
{
	NoOp,
	SetPitchAxis,
	SetPitchDiscrete,
	AdjustPitchTrim,
	SetRollAxis,
	SetRollDiscrete,
	AdjustRollTrim,
	SetYawAxis,
	SetYawDiscrete,
	AdjustYawTrim,
	ResetTrim,
	ToggleFbwCat,
	SetFbwCat1,
	SetFbwCat3,
	SetGLimiterOverride,
	ToggleGLimiterOverride,
	SetBothEngines,
	SetLeftEngine,
	SetRightEngine,
	SetCommonThrottleAxis,
	SetLeftThrottleAxis,
	SetRightThrottleAxis,
	StepCommonThrottle,
	StepLeftThrottle,
	StepRightThrottle,
	ToggleAirbrake,
	SetAirbrake,
	ToggleFlaps,
	SetFlapsUp,
	SetFlapsAuto,
	SetFlapsDown,
	ToggleGear,
	SetGear,
	ToggleNoseWheelSteering,
	SetNoseWheelSteering,
	SetBrake,
	SetLeftBrake,
	SetRightBrake
};

struct Command
{
	CommandId id = CommandId::NoOp;
	double value = 0.0;
};
}
