#pragma once

#include "../../../Core/Contracts/Diagnostics/DebugTelemetry.h"
#include "DebugTelemetryStatus.h"

#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <vector>

namespace DcsBridge
{
namespace Internal
{
inline constexpr std::uint32_t kDebugCsvSampleRateHz = 64;

class DebugTelemetryHub final : public Core::DebugTelemetrySink
{
public:
	explicit DebugTelemetryHub(
		Core::DebugTelemetryPublishErrorPolicy error_policy)
		: DebugTelemetrySink(error_policy)
	{
	}

	void begin_flight();
	void seal_schema();
	void release_flight();

	std::vector<Core::DebugTelemetryChannelDescriptor> schema() const;
	Core::DebugTelemetrySnapshot latest_snapshot() const;
	DebugTelemetryHubStatus status() const;
	std::vector<Core::DebugTelemetrySample> sample_through(
		Core::DebugSimulationTime target_time);

protected:
	Core::DebugTelemetryChannelId declare_channel_impl(
		Core::DebugTelemetryChannelDescriptor descriptor) override;
	void publish_impl(
		Core::DebugTelemetryChannelId id,
		Core::DebugSimulationTime time,
		Core::DebugTelemetryValue value) override;
	void report_publish_failure_impl(
		const Core::DebugTelemetryPublishFailure& failure) noexcept override;

private:
	struct TimedUpdate
	{
		Core::DebugTelemetryChannelId id = 0;
		Core::DebugSimulationTime time = {};
		Core::DebugTelemetryValue value;
	};

	Core::DebugTelemetryChannelId declare_new_channel(
		Core::DebugTelemetryChannelDescriptor descriptor);
	Core::DebugTelemetryChannelId redeclare_channel(
		std::size_t index,
		const Core::DebugTelemetryChannelDescriptor& descriptor);
	void validate_publish(
		Core::DebugTelemetryChannelId id,
		Core::DebugSimulationTime time,
		const Core::DebugTelemetryValue& value) const;
	void apply_updates_through(Core::DebugSimulationTime sample_time);
	static Core::DebugSimulationTime sample_time(std::uint64_t sequence);

	mutable std::mutex mutex_;
	std::vector<Core::DebugTelemetryChannelDescriptor> channels_;
	std::vector<bool> declared_this_flight_;
	std::vector<std::optional<Core::DebugTelemetryValue>> latest_values_;
	std::vector<std::optional<Core::DebugTelemetryValue>> sampled_values_;
	std::deque<TimedUpdate> updates_;
	std::optional<Core::DebugSimulationTime> last_publish_time_;
	std::optional<Core::DebugSimulationTime> last_sample_request_;
	std::uint64_t next_sample_sequence_ = 0;
	std::uint64_t failure_sequence_ = 0;
	std::uint64_t flight_failure_count_ = 0;
	Core::DebugTelemetryChannelId failure_channel_id_ = 0;
	Core::DebugSimulationTime failure_time_ = {};
	bool publication_failed_ = false;
	char failure_message_[kDebugTelemetryFailureMessageCapacity] = {};
	bool schema_locked_ = false;
	bool flight_active_ = false;
	bool schema_sealed_ = false;
};
}
}
