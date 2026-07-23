#pragma once

#include <exception>

namespace DcsBridge
{
namespace Internal
{
class EventLog;

void report_abi_exception(
	const char* callback,
	const std::exception_ptr& exception,
	EventLog* event_log) noexcept;
}
}

#define FCK1C_ABI_CATCH_VOID(callback, event_log, cleanup) \
	catch (...) \
	{ \
		(void)(cleanup); \
		::DcsBridge::Internal::report_abi_exception( \
			callback, std::current_exception(), event_log); \
	}

#define FCK1C_ABI_CATCH_RETURN(callback, event_log, neutral, cleanup) \
	catch (...) \
	{ \
		(void)(cleanup); \
		::DcsBridge::Internal::report_abi_exception( \
			callback, std::current_exception(), event_log); \
		return neutral; \
	}
