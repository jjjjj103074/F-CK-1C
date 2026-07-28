#pragma once

#include "ExecutionError.h"

#include <exception>
#include <string>
#include <string_view>
#include <utility>

namespace Core
{
namespace Diagnostics
{
inline constexpr const char* kUnknownExceptionReason =
	"unknown C++ exception";

struct ExecutionScope
{
	ExecutionOwnerType owner_type;
	std::string_view owner;
	std::string_view operation;
};

template <typename Action>
decltype(auto) invoke_with_execution_context(
	const ExecutionScope& scope,
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
			scope.owner_type,
			std::string(scope.owner),
			std::string(scope.operation),
			error.what()
		});
	}
	catch (...)
	{
		throw ExecutionError({
			scope.owner_type,
			std::string(scope.owner),
			std::string(scope.operation),
			kUnknownExceptionReason
		});
	}
}
}
}
