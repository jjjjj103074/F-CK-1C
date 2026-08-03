#include "DebugTelemetryHub.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <string>

namespace
{
constexpr std::uint64_t kNanosecondsPerSecond = 1'000'000'000ULL;

bool is_blank(const std::string& value)
{
	return value.empty() || std::all_of(
		value.begin(),
		value.end(),
		[](unsigned char character) { return std::isspace(character) != 0; });
}

bool descriptor_matches(
	const Core::DebugTelemetryChannelDescriptor& left,
	const Core::DebugTelemetryChannelDescriptor& right)
{
	return left.name == right.name && left.label == right.label &&
		left.value_type == right.value_type && left.unit == right.unit &&
		left.description == right.description;
}

Core::DebugTelemetryValueType value_type(
	const Core::DebugTelemetryValue& value)
{
	if (std::holds_alternative<double>(value))
		return Core::DebugTelemetryValueType::Double;
	if (std::holds_alternative<std::int64_t>(value))
		return Core::DebugTelemetryValueType::SignedInteger;
	if (std::holds_alternative<bool>(value))
		return Core::DebugTelemetryValueType::Boolean;
	if (std::holds_alternative<std::string>(value))
		return Core::DebugTelemetryValueType::Text;
	throw std::logic_error("Unknown debug telemetry value type.");
}

void validate_descriptor(
	const Core::DebugTelemetryChannelDescriptor& descriptor)
{
	if (is_blank(descriptor.name) || is_blank(descriptor.label))
	{
		throw std::logic_error(
			"Debug telemetry channel requires a name and label.");
	}
}
}

namespace DcsBridge
{
namespace Internal
{
void DebugTelemetryHub::begin_flight()
{
	std::lock_guard<std::mutex> lock(mutex_);
	declared_this_flight_.assign(channels_.size(), false);
	latest_values_.assign(channels_.size(), std::nullopt);
	sampled_values_.assign(channels_.size(), std::nullopt);
	updates_.clear();
	last_publish_time_.reset();
	last_sample_request_.reset();
	next_sample_sequence_ = 0;
	flight_failure_count_ = 0;
	publication_failed_ = false;
	failure_message_[0] = '\0';
	flight_active_ = true;
	schema_sealed_ = false;
}

void DebugTelemetryHub::seal_schema()
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (!flight_active_ || schema_sealed_)
	{
		throw std::logic_error(
			"Debug telemetry schema cannot be sealed now.");
	}
	if (schema_locked_ && std::find(
		declared_this_flight_.begin(),
		declared_this_flight_.end(),
		false) != declared_this_flight_.end())
	{
		throw std::logic_error(
			"Debug telemetry schema omitted a previously declared channel.");
	}
	schema_locked_ = true;
	schema_sealed_ = true;
}

void DebugTelemetryHub::release_flight()
{
	std::lock_guard<std::mutex> lock(mutex_);
	latest_values_.assign(channels_.size(), std::nullopt);
	sampled_values_.assign(channels_.size(), std::nullopt);
	updates_.clear();
	last_publish_time_.reset();
	last_sample_request_.reset();
	next_sample_sequence_ = 0;
	flight_active_ = false;
	schema_sealed_ = false;
}

Core::DebugTelemetryChannelId DebugTelemetryHub::declare_channel_impl(
	Core::DebugTelemetryChannelDescriptor descriptor)
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (!flight_active_ || schema_sealed_)
	{
		throw std::logic_error(
			"Debug telemetry channels must be declared during flight setup.");
	}
	validate_descriptor(descriptor);
	const auto existing = std::find_if(
		channels_.begin(),
		channels_.end(),
		[&descriptor](const auto& channel)
		{
			return channel.name == descriptor.name;
		});
	if (existing == channels_.end())
	{
		return declare_new_channel(std::move(descriptor));
	}
	return redeclare_channel(
		static_cast<std::size_t>(existing - channels_.begin()),
		descriptor);
}

Core::DebugTelemetryChannelId DebugTelemetryHub::declare_new_channel(
	Core::DebugTelemetryChannelDescriptor descriptor)
{
	if (schema_locked_)
	{
		throw std::logic_error(
			"Debug telemetry schema cannot add channels after it is locked.");
	}
	if (channels_.size() >=
		static_cast<std::size_t>(
			(std::numeric_limits<Core::DebugTelemetryChannelId>::max)()))
	{
		throw std::overflow_error(
			"Debug telemetry channel ID range was exhausted.");
	}
	const auto id = static_cast<Core::DebugTelemetryChannelId>(channels_.size());
	channels_.push_back(std::move(descriptor));
	declared_this_flight_.push_back(true);
	latest_values_.push_back(std::nullopt);
	sampled_values_.push_back(std::nullopt);
	return id;
}

Core::DebugTelemetryChannelId DebugTelemetryHub::redeclare_channel(
	std::size_t index,
	const Core::DebugTelemetryChannelDescriptor& descriptor)
{
	if (declared_this_flight_[index])
	{
		throw std::logic_error(
			"Debug telemetry channel name was declared more than once: " +
			descriptor.name);
	}
	if (!descriptor_matches(channels_[index], descriptor))
	{
		throw std::logic_error(
			"Debug telemetry channel changed its locked descriptor: " +
			descriptor.name);
	}
	declared_this_flight_[index] = true;
	return static_cast<Core::DebugTelemetryChannelId>(index);
}

void DebugTelemetryHub::publish_impl(
	Core::DebugTelemetryChannelId id,
	Core::DebugSimulationTime time,
	Core::DebugTelemetryValue value)
{
	std::lock_guard<std::mutex> lock(mutex_);
	validate_publish(id, time, value);
	latest_values_[id] = value;
	updates_.push_back({ id, time, std::move(value) });
	last_publish_time_ = time;
}

void DebugTelemetryHub::validate_publish(
	Core::DebugTelemetryChannelId id,
	Core::DebugSimulationTime time,
	const Core::DebugTelemetryValue& value) const
{
	if (!flight_active_ || id >= channels_.size() ||
		!declared_this_flight_[id])
	{
		throw std::logic_error(
			"Debug telemetry publish used an unavailable channel.");
	}
	if (time.count() < 0 ||
		(last_publish_time_ && time < *last_publish_time_))
	{
		throw std::logic_error(
			"Debug telemetry publish time must be monotonic.");
	}
	if (channels_[id].value_type != value_type(value))
	{
		throw std::logic_error(
			"Debug telemetry publish value type does not match its channel.");
	}
}

std::vector<Core::DebugTelemetryChannelDescriptor>
DebugTelemetryHub::schema() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return channels_;
}

Core::DebugTelemetrySnapshot DebugTelemetryHub::latest_snapshot() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return { channels_, latest_values_ };
}

DebugTelemetryHubStatus DebugTelemetryHub::status() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	DebugTelemetryHubStatus result;
	result.failure_sequence = failure_sequence_;
	result.flight_failure_count = flight_failure_count_;
	result.channel_id = failure_channel_id_;
	result.simulation_time_ns = failure_time_.count();
	result.publication_failed = publication_failed_;
	snprintf(
		result.message.data(),
		result.message.size(),
		"%s",
		failure_message_);
	return result;
}

std::vector<Core::DebugTelemetrySample>
DebugTelemetryHub::sample_through(Core::DebugSimulationTime target_time)
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (!flight_active_ || !schema_sealed_)
	{
		throw std::logic_error(
			"Debug telemetry sampling requires a sealed active flight.");
	}
	if (target_time.count() < 0 ||
		(last_sample_request_ && target_time < *last_sample_request_))
	{
		throw std::logic_error(
			"Debug telemetry sample target time must be monotonic.");
	}
	std::vector<Core::DebugTelemetrySample> result;
	while (sample_time(next_sample_sequence_) <= target_time)
	{
		const Core::DebugSimulationTime time =
			sample_time(next_sample_sequence_);
		apply_updates_through(time);
		result.push_back({ next_sample_sequence_, time, sampled_values_ });
		++next_sample_sequence_;
	}
	last_sample_request_ = target_time;
	return result;
}

void DebugTelemetryHub::apply_updates_through(
	Core::DebugSimulationTime sample_time_value)
{
	while (!updates_.empty() && updates_.front().time <= sample_time_value)
	{
		TimedUpdate& update = updates_.front();
		sampled_values_[update.id] = std::move(update.value);
		updates_.pop_front();
	}
}

Core::DebugSimulationTime DebugTelemetryHub::sample_time(
	std::uint64_t sequence)
{
	const std::uint64_t seconds = sequence / kDebugCsvSampleRateHz;
	const std::uint64_t remainder = sequence % kDebugCsvSampleRateHz;
	const auto maximum = static_cast<std::uint64_t>(
		(std::numeric_limits<std::int64_t>::max)());
	if (seconds > maximum / kNanosecondsPerSecond)
	{
		throw std::overflow_error(
			"Debug telemetry sample time exceeded its range.");
	}
	const std::uint64_t whole_nanoseconds =
		seconds * kNanosecondsPerSecond;
	const std::uint64_t partial_nanoseconds =
		(remainder * kNanosecondsPerSecond) / kDebugCsvSampleRateHz;
	if (partial_nanoseconds > maximum - whole_nanoseconds)
	{
		throw std::overflow_error(
			"Debug telemetry sample time exceeded its range.");
	}
	const std::uint64_t total = whole_nanoseconds + partial_nanoseconds;
	return Core::DebugSimulationTime(static_cast<std::int64_t>(total));
}

void DebugTelemetryHub::report_publish_failure_impl(
	const Core::DebugTelemetryPublishFailure& failure) noexcept
{
	try
	{
		std::lock_guard<std::mutex> lock(mutex_);
		++flight_failure_count_;
		if (publication_failed_) return;
		++failure_sequence_;
		failure_channel_id_ = failure.channel_id;
		failure_time_ = failure.simulation_time;
		publication_failed_ = true;
		const char* source = failure.message == nullptr
			? "Debug telemetry publication failed without a message."
			: failure.message;
		snprintf(
			failure_message_, sizeof(failure_message_), "%s", source);
	}
	catch (...)
	{
		std::terminate();
	}
}
}
}
