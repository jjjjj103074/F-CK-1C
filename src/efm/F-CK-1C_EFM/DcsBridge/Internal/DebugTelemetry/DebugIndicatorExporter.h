#pragma once

#include "DebugTelemetryHub.h"
#include "DebugTelemetryStatus.h"
#include "../CockpitParameterEndpoint.h"

#include <array>
#include <mutex>
#include <string>

namespace DcsBridge
{
namespace Internal
{
inline constexpr std::size_t kDebugIndicatorColumnCount = 2;
inline constexpr std::size_t kDebugIndicatorBlocksPerColumn = 3;
inline constexpr std::size_t kDebugIndicatorRowsPerBlock = 10;
inline constexpr std::size_t kDebugIndicatorBlockCount =
	kDebugIndicatorColumnCount * kDebugIndicatorBlocksPerColumn;
inline constexpr std::size_t kDebugIndicatorDataCapacity =
	kDebugIndicatorBlockCount * kDebugIndicatorRowsPerBlock;
inline constexpr std::size_t kDebugIndicatorStatusIndex =
	kDebugIndicatorBlockCount;
inline constexpr std::size_t kDebugIndicatorTextParameterCount =
	kDebugIndicatorBlockCount + 1;

struct DebugIndicatorPayload
{
	std::array<std::string, kDebugIndicatorBlockCount> blocks;
	std::string status;
};

class DebugIndicatorExporter final
{
public:
	DebugIndicatorExporter(
		cockpit_param_api api,
		const DebugTelemetryHub& hub);

	CockpitParameterEvents begin_flight();
	CockpitParameterEvents release_flight();
	CockpitParameterEvents toggle();
	CockpitParameterEvents export_latest(const DebugCsvStatus& csv_status);
	bool visible() const;

private:
	CockpitParameterEvents set_visible(bool value);
	CockpitParameterEvents clear_text(bool force);
	CockpitParameterEvents write_payload(
		const DebugIndicatorPayload& payload,
		bool force);
	CockpitParameterEvents write_text(
		std::size_t index,
		const std::string& text,
		bool force);
	DebugIndicatorPayload format_latest(const DebugCsvStatus& csv_status) const;
	void invalidate_text_cache();

	const cockpit_param_api api_;
	const DebugTelemetryHub& hub_;
	CockpitParameterEndpoint visible_parameter_;
	std::array<CockpitParameterEndpoint,
		kDebugIndicatorTextParameterCount> text_parameters_;
	std::array<std::string, kDebugIndicatorTextParameterCount> cached_text_;
	std::array<bool, kDebugIndicatorTextParameterCount> text_cache_valid_ = {};
	mutable std::mutex mutex_;
	bool visible_ = false;
};
}
}
