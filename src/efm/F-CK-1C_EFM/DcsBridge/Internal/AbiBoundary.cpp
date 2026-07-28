#include "AbiBoundary.h"

#include "EventLog.h"
#include "../../Core/Diagnostics/ExecutionError.h"

#include <cstdio>
#include <exception>
#include <windows.h>

namespace
{
constexpr size_t kAbiErrorMessageCapacity = 1400;
constexpr const char* kUnknownException = "unknown C++ exception";

struct AbiErrorOutput
{
	char* data;
	size_t capacity;
	const char* callback;
};

const char* owner_type_name(Core::ExecutionOwnerType owner_type)
{
	switch (owner_type)
	{
	case Core::ExecutionOwnerType::System:
		return "system";
	case Core::ExecutionOwnerType::SimulationModel:
		return "simulation_model";
	}
	return "unknown";
}

void format_standard_exception(
	const AbiErrorOutput& output,
	const char* reason) noexcept
{
	std::snprintf(
		output.data,
		output.capacity,
		"callback=%s unhandled_exception=%s caught_at=c_abi_boundary",
		output.callback,
		reason);
}

void format_execution_error(
	const AbiErrorOutput& output,
	const Core::ExecutionError& error) noexcept
{
	const Core::ExecutionErrorDetails& details = error.details();
	std::snprintf(
		output.data,
		output.capacity,
		"callback=%s source=%s owner=%s operation=%s reason=%s "
		"caught_at=c_abi_boundary",
		output.callback,
		owner_type_name(details.owner_type),
		details.owner.c_str(),
		details.operation.c_str(),
		details.reason.c_str());
}

void format_exception(
	const AbiErrorOutput& output,
	const std::exception_ptr& exception) noexcept
{
	if (!exception)
	{
		format_standard_exception(output, kUnknownException);
		return;
	}
	try
	{
		std::rethrow_exception(exception);
	}
	catch (const Core::ExecutionError& error)
	{
		format_execution_error(output, error);
	}
	catch (const std::exception& error)
	{
		format_standard_exception(output, error.what());
	}
	catch (...)
	{
		format_standard_exception(output, kUnknownException);
	}
}

void write_debug_output(const char* message) noexcept
{
	OutputDebugStringA("[F-CK-1C EFM] ");
	OutputDebugStringA(message);
	OutputDebugStringA("\n");
}
}

namespace DcsBridge
{
namespace Internal
{
void report_abi_exception(
	const char* callback,
	const std::exception_ptr& exception,
	EventLog* event_log) noexcept
{
	char message[kAbiErrorMessageCapacity];
	format_exception({ message, sizeof(message), callback }, exception);

	if (event_log != nullptr)
	{
		try
		{
			if (event_log->write({ EventLevel::Error, std::nullopt, message }))
			{
				return;
			}
		}
		catch (...)
		{
		}
	}
	write_debug_output(message);
}
}
}
