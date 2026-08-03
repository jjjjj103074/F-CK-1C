#pragma once

#include "DebugTelemetryTypes.h"

#include <exception>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace Core
{
class DebugTelemetrySink;

enum class DebugTelemetryPublishErrorPolicy
{
	IsolateAndReport,
	ReportAndRethrow
};

struct DebugTelemetryPublishFailure
{
	DebugTelemetryChannelId channel_id = 0;
	DebugSimulationTime simulation_time = {};
	const char* message = nullptr;
};

template <typename T>
class DebugTelemetryChannel final
{
public:
	DebugTelemetryChannel() = default;

	void publish(DebugSimulationTime time, const T& value) const;
	bool valid() const noexcept { return sink_ != nullptr; }

private:
	DebugTelemetryChannel(
		DebugTelemetrySink& sink,
		DebugTelemetryChannelId id)
		: sink_(&sink), id_(id)
	{
	}

	DebugTelemetrySink* sink_ = nullptr;
	DebugTelemetryChannelId id_ = 0;

	friend class DebugTelemetrySink;
};

template <typename T>
struct DebugTelemetryType;

template <>
struct DebugTelemetryType<double>
{
	static constexpr DebugTelemetryValueType value =
		DebugTelemetryValueType::Double;
};

template <>
struct DebugTelemetryType<std::int64_t>
{
	static constexpr DebugTelemetryValueType value =
		DebugTelemetryValueType::SignedInteger;
};

template <>
struct DebugTelemetryType<bool>
{
	static constexpr DebugTelemetryValueType value =
		DebugTelemetryValueType::Boolean;
};

template <>
struct DebugTelemetryType<std::string>
{
	static constexpr DebugTelemetryValueType value =
		DebugTelemetryValueType::Text;
};

class DebugTelemetrySink
{
public:
	explicit DebugTelemetrySink(DebugTelemetryPublishErrorPolicy error_policy)
		: error_policy_(error_policy)
	{
	}

	virtual ~DebugTelemetrySink() = default;

	template <typename T>
	DebugTelemetryChannel<T> declare_channel(
		DebugTelemetryChannelDescriptor descriptor)
	{
		descriptor.value_type = DebugTelemetryType<T>::value;
		return DebugTelemetryChannel<T>(
			*this,
			declare_channel_impl(std::move(descriptor)));
	}

protected:
	virtual DebugTelemetryChannelId declare_channel_impl(
		DebugTelemetryChannelDescriptor descriptor) = 0;
	virtual void publish_impl(
		DebugTelemetryChannelId id,
		DebugSimulationTime time,
		DebugTelemetryValue value) = 0;
	virtual void report_publish_failure_impl(
		const DebugTelemetryPublishFailure& failure) noexcept = 0;

private:
	template <typename T>
	void publish_value(
		DebugTelemetryChannelId id,
		DebugSimulationTime time,
		const T& value)
	{
		try
		{
			publish_impl(id, time, DebugTelemetryValue(value));
		}
		catch (const std::exception& error)
		{
			report_publish_failure_impl({ id, time, error.what() });
			if (error_policy_ ==
				DebugTelemetryPublishErrorPolicy::ReportAndRethrow)
			{
				throw;
			}
		}
		catch (...)
		{
			report_publish_failure_impl({
				id,
				time,
				"Unknown debug telemetry publication failure." });
			if (error_policy_ ==
				DebugTelemetryPublishErrorPolicy::ReportAndRethrow)
			{
				throw;
			}
		}
	}

	const DebugTelemetryPublishErrorPolicy error_policy_;

	template <typename T>
	friend class DebugTelemetryChannel;
};

template <typename T>
void DebugTelemetryChannel<T>::publish(
	DebugSimulationTime time,
	const T& value) const
{
	if (sink_ == nullptr)
	{
		throw std::logic_error(
			"Debug telemetry channel was not declared.");
	}
	sink_->publish_value(id_, time, value);
}
}
