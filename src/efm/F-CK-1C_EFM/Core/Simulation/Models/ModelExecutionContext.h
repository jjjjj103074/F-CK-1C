#pragma once

#include "../../Diagnostics/ExecutionContext.h"

#include <utility>

namespace Core
{
namespace Simulation
{
namespace Detail
{
inline constexpr const char* kModelCreateOperation = "create";
inline constexpr const char* kModelStepOperation = "step";
inline constexpr const char* kAerodynamicsOwner = "aerodynamics";
inline constexpr const char* kPropulsionOwner = "propulsion";
inline constexpr const char* kGroundInteractionOwner = "ground_interaction";
inline constexpr const char* kMassPropertiesOwner = "mass_properties";

template <typename Action>
decltype(auto) invoke_model_action(
	const char* owner,
	const char* operation,
	Action&& action)
{
	return Diagnostics::invoke_with_execution_context(
		{ ExecutionOwnerType::SimulationModel, owner, operation },
		std::forward<Action>(action));
}
}
}
}
