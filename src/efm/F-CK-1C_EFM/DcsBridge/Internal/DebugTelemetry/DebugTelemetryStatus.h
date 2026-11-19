#pragma once

#include "../../../Core/Contracts/Diagnostics/DebugTelemetryTypes.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace DcsBridge
{
namespace Internal
{
inline constexpr std::size_t kDebugTelemetryFailureMessageCapacity = 256;

struct DebugTelemetryHubStatus
{
	std::uint64_t failure_sequence = 0;
	std::uint64_t flight_failure_count = 0;
	Core::DebugTelemetryChannelId channel_id = 0;
	std::int64_t simulation_time_ns = 0;
	bool publication_failed = false;
	std::array<char, kDebugTelemetryFailureMessageCapacity> message = {};
};

struct DebugCsvStatus
{
	bool ready = false;
	int error_code = 0;
	const char* failed_operation = nullptr;
};
}
}
