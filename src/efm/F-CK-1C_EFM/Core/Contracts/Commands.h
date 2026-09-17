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
	SetRightBrake,
	SetTriggerFirstStage,
	StartCmsForward,
	StartCmsAft,
	StartCmsLeft,
	StartCmsRight,
	PressCms,
	SetTriggerSecondStage,
	SetMasterArmOn,
	SetMasterArmOff,
	SetMasterArmSim,
	SelectDogfightMode,
	SetMissileUncage,
	SetWeaponRelease,
	SetTmsUp,
	PressTmsDown,
	PressTmsLeft,
	PressTmsRight,
	SelectNavigationMode,
	SelectMissileOverride,
	EngageAutopilot,
	DisengageAutopilot,
	SetAutopilotBypass,
	SelectAutopilotPitchAttitudeHold,
	SelectAutopilotAltitudeHold,
	SelectAutopilotRollAttitudeHold,
	SelectAutopilotHeadingSelect,
	IncreaseAutopilotHeadingSelect,
	DecreaseAutopilotHeadingSelect,
	ToggleAutoThrottle,
	EngageAutoThrottle,
	DisengageAutoThrottle,
	IncreaseAutopilotSpeed,
	DecreaseAutopilotSpeed,
	ToggleThrustCutTest,
	EnableThrustCutTest,
	DisableThrustCutTest
};

struct Command
{
	// Command payloads are normalized DCS command authority. Primary controls
	// use the Core convention: pull, right roll, and left yaw are positive. Raw
	// DCS signs are adapted before this contract is published.
	CommandId id = CommandId::NoOp;
	double value_normalized = 0.0;
};
}
