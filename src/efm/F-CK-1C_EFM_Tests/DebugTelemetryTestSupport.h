#pragma once

#include "Core/Contracts/Diagnostics/DebugTelemetry.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace Tests
{
class DisabledDebugTelemetry final : public Core::DebugTelemetrySink
{
public:
	DisabledDebugTelemetry()
		: DebugTelemetrySink(
			Core::DebugTelemetryPublishErrorPolicy::IsolateAndReport)
	{
	}

protected:
	Core::DebugTelemetryChannelId declare_channel_impl(
		Core::DebugTelemetryChannelDescriptor) override
	{
		return kDiscardedChannelId;
	}

	void publish_impl(
		Core::DebugTelemetryChannelId,
		Core::DebugSimulationTime,
		Core::DebugTelemetryValue) override
	{
	}

	void report_publish_failure_impl(
		const Core::DebugTelemetryPublishFailure&) noexcept override
	{
	}

private:
	static constexpr Core::DebugTelemetryChannelId kDiscardedChannelId = 0;
};

struct RecordedDebugTelemetryUpdate
{
	Core::DebugTelemetryChannelId id = 0;
	Core::DebugSimulationTime time = {};
	Core::DebugTelemetryValue value;
};

class RecordingDebugTelemetry final : public Core::DebugTelemetrySink
{
public:
	RecordingDebugTelemetry()
		: DebugTelemetrySink(
			Core::DebugTelemetryPublishErrorPolicy::ReportAndRethrow)
	{
	}

	const std::vector<Core::DebugTelemetryChannelDescriptor>& channels() const
	{
		return channels_;
	}

	const std::vector<RecordedDebugTelemetryUpdate>& updates() const
	{
		return updates_;
	}

protected:
	Core::DebugTelemetryChannelId declare_channel_impl(
		Core::DebugTelemetryChannelDescriptor descriptor) override
	{
		const auto id = static_cast<Core::DebugTelemetryChannelId>(
			channels_.size());
		channels_.push_back(std::move(descriptor));
		return id;
	}

	void publish_impl(
		Core::DebugTelemetryChannelId id,
		Core::DebugSimulationTime time,
		Core::DebugTelemetryValue value) override
	{
		updates_.push_back({ id, time, std::move(value) });
	}

	void report_publish_failure_impl(
		const Core::DebugTelemetryPublishFailure&) noexcept override
	{
		publication_failed_ = true;
	}

private:
	std::vector<Core::DebugTelemetryChannelDescriptor> channels_;
	std::vector<RecordedDebugTelemetryUpdate> updates_;
	bool publication_failed_ = false;
};

inline DisabledDebugTelemetry& disabled_debug_telemetry()
{
	static DisabledDebugTelemetry telemetry;
	return telemetry;
}
}
