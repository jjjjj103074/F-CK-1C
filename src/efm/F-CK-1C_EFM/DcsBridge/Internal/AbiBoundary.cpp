#include "AbiBoundary.h"

#include "EventLog.h"

#include <cstdio>
#include <exception>
#include <windows.h>

namespace
{
constexpr size_t kAbiErrorMessageCapacity = 1400;
constexpr const char* kUnknownException = "unknown C++ exception";

const char* exception_message(const std::exception_ptr& exception) noexcept
{
	if (!exception)
	{
		return kUnknownException;
	}
	try
	{
		std::rethrow_exception(exception);
	}
	catch (const std::exception& error)
	{
		return error.what();
	}
	catch (...)
	{
		return kUnknownException;
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
	std::snprintf(
		message,
		sizeof(message),
		"callback=%s unhandled_exception=%s caught_at=c_abi_boundary",
		callback,
		exception_message(exception));

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
