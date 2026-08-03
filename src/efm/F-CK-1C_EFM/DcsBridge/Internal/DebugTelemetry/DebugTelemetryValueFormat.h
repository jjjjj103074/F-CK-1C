#pragma once

#include "../../../Core/Contracts/Diagnostics/DebugTelemetryTypes.h"

#include <charconv>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>

namespace DcsBridge
{
namespace Internal
{
namespace Detail
{
inline constexpr std::size_t kNumericBufferCapacity = 64;
inline constexpr int kRoundTripDoubleDigits =
	std::numeric_limits<double>::max_digits10;

inline std::string format_number(double value)
{
	char buffer[kNumericBufferCapacity];
	const std::to_chars_result result = std::to_chars(
		buffer,
		buffer + sizeof(buffer),
		value,
		std::chars_format::general,
		kRoundTripDoubleDigits);
	if (result.ec != std::errc())
	{
		throw std::runtime_error(
			"Debug telemetry could not format a double.");
	}
	return std::string(buffer, result.ptr);
}

inline std::string format_number(std::int64_t value)
{
	char buffer[kNumericBufferCapacity];
	const std::to_chars_result result =
		std::to_chars(buffer, buffer + sizeof(buffer), value);
	if (result.ec != std::errc())
	{
		throw std::runtime_error(
			"Debug telemetry could not format an integer.");
	}
	return std::string(buffer, result.ptr);
}

struct DebugTelemetryValueVisitor
{
	std::string operator()(double value) const { return format_number(value); }
	std::string operator()(std::int64_t value) const
	{
		return format_number(value);
	}
	std::string operator()(bool value) const
	{
		return value ? "True" : "False";
	}
	std::string operator()(const std::string& value) const { return value; }
};
}

inline std::string format_debug_telemetry_value(
	const Core::DebugTelemetryValue& value)
{
	return std::visit(Detail::DebugTelemetryValueVisitor {}, value);
}
}
}
