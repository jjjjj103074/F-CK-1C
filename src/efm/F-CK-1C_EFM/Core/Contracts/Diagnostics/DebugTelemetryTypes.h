#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace Core
{
using DebugSimulationTime = std::chrono::nanoseconds;
using DebugTelemetryChannelId = std::uint32_t;

enum class DebugTelemetryValueType
{
	Double,
	SignedInteger,
	Boolean,
	Text
};

struct DebugTelemetryChannelDescriptor
{
	std::string name;
	std::string label;
	DebugTelemetryValueType value_type = DebugTelemetryValueType::Double;
	std::string unit;
	std::string description;
};

using DebugTelemetryValue = std::variant<
	double,
	std::int64_t,
	bool,
	std::string>;

struct DebugTelemetrySnapshot
{
	std::vector<DebugTelemetryChannelDescriptor> channels;
	std::vector<std::optional<DebugTelemetryValue>> values;
};

struct DebugTelemetrySample
{
	std::uint64_t sequence = 0;
	DebugSimulationTime simulation_time = {};
	std::vector<std::optional<DebugTelemetryValue>> values;
};
}
