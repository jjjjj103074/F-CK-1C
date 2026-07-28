#include "DcsCommandRouter.h"

#include "../../DcsIds/Commands.h"

#include <cmath>
#include <cstddef>
#include <iterator>

namespace
{
constexpr double kPitchTrimStep = 0.0015;
constexpr double kRollTrimStep = 0.001;
constexpr double kYawTrimStep = 0.001;
constexpr double kThrottleStep = 0.0075;

enum class ValueRule
{
	PassThrough,
	Constant,
	PressOnly
};

struct CommandBinding
{
	int dcs_id;
	Core::CommandId command_id;
	ValueRule value_rule;
	double constant;
};

#define BIND_INPUT(id, action) \
	{ DcsIds::Commands::id, Core::CommandId::action, ValueRule::PassThrough, 0.0 }
#define BIND_CONST(id, action, value) \
	{ DcsIds::Commands::id, Core::CommandId::action, ValueRule::Constant, value }
#define BIND_PRESS(id, action) \
	{ DcsIds::Commands::id, Core::CommandId::action, ValueRule::PressOnly, 0.0 }

constexpr CommandBinding kBindings[] = {
	BIND_INPUT(JoystickPitch, SetPitchAxis),
	BIND_CONST(PitchUp, SetPitchDiscrete, 1.0),
	BIND_CONST(PitchUpStop, SetPitchDiscrete, 0.0),
	BIND_CONST(PitchDown, SetPitchDiscrete, -1.0),
	BIND_CONST(PitchDownStop, SetPitchDiscrete, 0.0),
	BIND_CONST(TrimUp, AdjustPitchTrim, kPitchTrimStep),
	BIND_CONST(TrimDown, AdjustPitchTrim, -kPitchTrimStep),
	BIND_INPUT(JoystickRoll, SetRollAxis),
	BIND_CONST(RollLeft, SetRollDiscrete, -1.0),
	BIND_CONST(RollLeftStop, SetRollDiscrete, 0.0),
	BIND_CONST(RollRight, SetRollDiscrete, 1.0),
	BIND_CONST(RollRightStop, SetRollDiscrete, 0.0),
	BIND_CONST(TrimLeft, AdjustRollTrim, -kRollTrimStep),
	BIND_CONST(TrimRight, AdjustRollTrim, kRollTrimStep),
	BIND_INPUT(PedalYaw, SetYawAxis),
	BIND_CONST(RudderLeft, SetYawDiscrete, 1.0),
	BIND_CONST(RudderLeftStop, SetYawDiscrete, 0.0),
	BIND_CONST(RudderRight, SetYawDiscrete, -1.0),
	BIND_CONST(RudderRightStop, SetYawDiscrete, 0.0),
	BIND_CONST(RudderTrimLeft, AdjustYawTrim, kYawTrimStep),
	BIND_CONST(RudderTrimRight, AdjustYawTrim, -kYawTrimStep),
	BIND_CONST(ResetTrim, ResetTrim, 0.0),
	BIND_PRESS(FBWCatToggle, ToggleFbwCat),
	BIND_PRESS(FBWCat1, SetFbwCat1),
	BIND_PRESS(FBWCat3, SetFbwCat3),
	BIND_PRESS(FBWGLimiterOverride, SetGLimiterOverride),
	BIND_PRESS(FBWGLimiterOverrideToggle, ToggleGLimiterOverride),
	BIND_CONST(EnginesOn, SetBothEngines, 1.0),
	BIND_CONST(LeftEngineOn, SetLeftEngine, 1.0),
	BIND_CONST(RightEngineOn, SetRightEngine, 1.0),
	BIND_CONST(EnginesOff, SetBothEngines, 0.0),
	BIND_CONST(LeftEngineOff, SetLeftEngine, 0.0),
	BIND_CONST(RightEngineOff, SetRightEngine, 0.0),
	BIND_INPUT(ThrottleAxis, SetCommonThrottleAxis),
	BIND_INPUT(ThrottleAxisLeft, SetLeftThrottleAxis),
	BIND_INPUT(ThrottleAxisRight, SetRightThrottleAxis),
	BIND_CONST(ThrottleIncrease, StepCommonThrottle, kThrottleStep),
	BIND_CONST(ThrottleLeftUp, StepLeftThrottle, kThrottleStep),
	BIND_CONST(ThrottleRightUp, StepRightThrottle, kThrottleStep),
	BIND_CONST(ThrottleDecrease, StepCommonThrottle, -kThrottleStep),
	BIND_CONST(ThrottleLeftDown, StepLeftThrottle, -kThrottleStep),
	BIND_CONST(ThrottleRightDown, StepRightThrottle, -kThrottleStep),
	BIND_CONST(ThrottleStop, NoOp, 0.0),
	BIND_CONST(AirBrakes, ToggleAirbrake, 0.0),
	BIND_CONST(AirBrakesOff, SetAirbrake, 0.0),
	BIND_CONST(AirBrakesOn, SetAirbrake, 1.0),
	BIND_CONST(AirBrakesAuto, NoOp, 0.0),
	BIND_CONST(AirBrakesUp, SetAirbrake, 0.0),
	BIND_CONST(AirBrakesDown, SetAirbrake, 1.0),
	BIND_CONST(FlapsToggle, ToggleFlaps, 0.0),
	BIND_CONST(FlapsDown, SetFlapsDown, 0.0),
	BIND_CONST(FlapsUp, SetFlapsUp, 0.0),
	BIND_CONST(FlapsAuto, SetFlapsAuto, 0.0),
	BIND_CONST(FlapsUpCmd, SetFlapsUp, 0.0),
	BIND_CONST(FlapsDownCmd, SetFlapsDown, 0.0),
	BIND_CONST(GearToggle, ToggleGear, 0.0),
	BIND_CONST(GearDown, SetGear, 1.0),
	BIND_CONST(GearUp, SetGear, 0.0),
	BIND_CONST(GearAuto, NoOp, 0.0),
	BIND_CONST(GearHandleUp, SetGear, 0.0),
	BIND_CONST(GearHandleDown, SetGear, 1.0),
	BIND_PRESS(NoseTurnToggle, ToggleNoseWheelSteering),
	BIND_CONST(NoseTurnUp, SetNoseWheelSteering, 0.0),
	BIND_CONST(NoseTurnAuto, SetNoseWheelSteering, 0.0),
	BIND_CONST(NoseTurnDown, SetNoseWheelSteering, 1.0),
	BIND_INPUT(WheelBrakeAxis, SetBrake),
	BIND_INPUT(WheelBrakeAxisLeft, SetLeftBrake),
	BIND_INPUT(WheelBrakeAxisRight, SetRightBrake),
	BIND_CONST(WheelBrakeOn, SetBrake, 1.0),
	BIND_CONST(WheelBrakeOff, SetBrake, 0.0),
	BIND_CONST(WheelBrakeLeftOn, SetLeftBrake, 1.0),
	BIND_CONST(WheelBrakeLeftOff, SetLeftBrake, 0.0),
	BIND_CONST(WheelBrakeRightOn, SetRightBrake, 1.0),
	BIND_CONST(WheelBrakeRightOff, SetRightBrake, 0.0)
};

#undef BIND_INPUT
#undef BIND_CONST
#undef BIND_PRESS

double mapped_value(const CommandBinding& binding, float input)
{
	switch (binding.value_rule)
	{
	case ValueRule::PassThrough: return input;
	case ValueRule::PressOnly: return 1.0;
	case ValueRule::Constant: return binding.constant;
	}
	return 0.0;
}

bool has_valid_rule(const CommandBinding& binding)
{
	switch (binding.value_rule)
	{
	case ValueRule::PassThrough:
	case ValueRule::PressOnly:
		return true;
	case ValueRule::Constant:
		return std::isfinite(binding.constant);
	}
	return false;
}

DcsBridge::CommandTableValidation find_duplicate_binding()
{
	for (std::size_t index = 0; index < std::size(kBindings); ++index)
	{
		for (std::size_t other = index + 1; other < std::size(kBindings); ++other)
		{
			if (kBindings[index].dcs_id == kBindings[other].dcs_id)
			{
				return { DcsBridge::CommandBindingError::DuplicateId, kBindings[index].dcs_id };
			}
		}
	}
	return {};
}

DcsBridge::CommandTableValidation inspect_command_bindings()
{
	for (const CommandBinding& binding : kBindings)
	{
		if (!has_valid_rule(binding))
		{
			return { DcsBridge::CommandBindingError::InvalidRule, binding.dcs_id };
		}
	}
	return find_duplicate_binding();
}

bool is_ignored_command(int command)
{
	for (const DcsIds::CommandRouting::Entry& entry :
		DcsIds::CommandRouting::CustomCommands)
	{
		if (entry.id == command)
		{
			return entry.route == DcsIds::CommandRouting::Route::Cockpit;
		}
	}
	for (const int ignored : DcsIds::CommandRouting::IgnoredDcsCommands)
	{
		if (ignored == command)
		{
			return true;
		}
	}
	return false;
}
}

namespace DcsBridge
{
CommandTableValidation validate_command_bindings()
{
	static const CommandTableValidation validation = []()
		{
			CommandTableValidation result = inspect_command_bindings();
			result.binding_count = std::size(kBindings);
			return result;
		}();
	return validation;
}

DcsCommandMapping map_command(int command, float value)
{
	if (is_ignored_command(command))
	{
		return { DcsCommandMappingStatus::IgnoredCommand };
	}
	if (!std::isfinite(value))
	{
		return { DcsCommandMappingStatus::InvalidValue };
	}
	const CommandTableValidation validation = validate_command_bindings();
	if (validation.error != CommandBindingError::None)
	{
		return {
			DcsCommandMappingStatus::InvalidBindingTable,
			{},
			validation
		};
	}
	for (const CommandBinding& binding : kBindings)
	{
		if (binding.dcs_id == command)
		{
			if (binding.value_rule == ValueRule::PressOnly && value <= 0.0f)
			{
				return { DcsCommandMappingStatus::IgnoredRelease };
			}
			return {
				DcsCommandMappingStatus::Mapped,
				{ binding.command_id, mapped_value(binding, value) }
			};
		}
	}
	return { DcsCommandMappingStatus::UnknownCommand };
}
}
