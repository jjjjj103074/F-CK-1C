#pragma once

#include "SystemPipeline.h"
#include "../Contracts/Diagnostics.h"

#include <exception>
#include <functional>
#include <string>
#include <utility>

namespace Core
{
namespace Systems
{
namespace Detail
{
inline constexpr const char* kSystemStepOperation = "step";
inline constexpr const char* kCommandOperation = "handle_command";
inline constexpr const char* kDamageOperation = "apply_damage";
inline constexpr const char* kRepairOperation = "repair";
inline constexpr const char* kReadFuelOperation = "read_fuel_state";
inline constexpr const char* kCurrentFuelOperation = "read_current_fuel";
inline constexpr const char* kSetInternalFuelOperation = "set_internal_fuel";
inline constexpr const char* kSetExternalFuelOperation = "set_external_fuel";
inline constexpr const char* kSuppressFuelOperation = "suppress_fuel_consumption";
inline constexpr const char* kUnknownExceptionReason = "unknown C++ exception";

template <typename Action>
decltype(auto) invoke_system_action(
	const std::string& system_id,
	const char* operation,
	Action&& action)
{
	try
	{
		return std::forward<Action>(action)();
	}
	catch (const ExecutionError&)
	{
		throw;
	}
	catch (const std::exception& error)
	{
		throw ExecutionError({
			ExecutionOwnerType::System,
			system_id,
			operation,
			error.what()
		});
	}
	catch (...)
	{
		throw ExecutionError({
			ExecutionOwnerType::System,
			system_id,
			operation,
			kUnknownExceptionReason
		});
	}
}

template <typename Result, typename... Arguments>
std::function<Result(Arguments...)> with_system_context(
	const std::string& system_id,
	const char* operation,
	std::function<Result(Arguments...)> handler)
{
	return [
		system_id,
		operation,
		handler = std::move(handler)
	](Arguments... arguments) mutable -> Result
	{
		return invoke_system_action(
			system_id,
			operation,
			[&]() -> Result
			{
				return handler(std::forward<Arguments>(arguments)...);
			});
	};
}

inline FuelManagementHandlers with_system_context(
	const std::string& system_id,
	FuelManagementHandlers handlers)
{
	return {
		with_system_context(
			system_id, kReadFuelOperation, std::move(handlers.read)),
		with_system_context(
			system_id, kCurrentFuelOperation, std::move(handlers.current_data)),
		with_system_context(
			system_id, kSetInternalFuelOperation, std::move(handlers.set_internal)),
		with_system_context(
			system_id, kSetExternalFuelOperation, std::move(handlers.set_external)),
		with_system_context(
			system_id, kSuppressFuelOperation,
			std::move(handlers.suppress_consumption))
	};
}
}
}
}
