#include "DebugIndicatorExporter.h"

#include "../../../DcsIds/CockpitParams.g.h"

#include <algorithm>
#include <charconv>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <system_error>

namespace
{
using Core::DebugTelemetryChannelDescriptor;
using Core::DebugTelemetryValue;
using DcsBridge::Internal::DebugIndicatorPayload;

constexpr std::size_t kNumericBufferCapacity = 64;
constexpr int kIndicatorSignificantDigits = 6;

std::string escape_line_text(const std::string& value)
{
	std::string result;
	result.reserve(value.size());
	for (const char character : value)
	{
		switch (character)
		{
		case '\n': result += "\\n"; break;
		case '\r': result += "\\r"; break;
		case '\t': result += "\\t"; break;
		default: result += character; break;
		}
	}
	return result;
}

std::string to_cockpit_font_text(const std::string& value)
{
	std::string result = value;
	for (char& character : result)
	{
		if (character >= 'a' && character <= 'z')
		{
			character = static_cast<char>(character - 'a' + 'A');
		}
	}
	return result;
}

std::string format_indicator_number(double value)
{
	char buffer[kNumericBufferCapacity];
	const std::to_chars_result result = std::to_chars(
		buffer,
		buffer + sizeof(buffer),
		value,
		std::chars_format::general,
		kIndicatorSignificantDigits);
	if (result.ec != std::errc())
	{
		throw std::runtime_error("Debug Indicator could not format a double.");
	}
	return std::string(buffer, result.ptr);
}

struct IndicatorValueVisitor
{
	std::string operator()(double value) const
	{
		return format_indicator_number(value);
	}
	std::string operator()(std::int64_t value) const
	{
		return std::to_string(value);
	}
	std::string operator()(bool value) const
	{
		return value ? "True" : "False";
	}
	std::string operator()(const std::string& value) const
	{
		return escape_line_text(value);
	}
};

std::string display_value(const std::optional<DebugTelemetryValue>& value)
{
	return value ? std::visit(IndicatorValueVisitor {}, *value) : "-";
}

std::string format_channel(
	const DebugTelemetryChannelDescriptor& channel,
	const std::optional<DebugTelemetryValue>& value)
{
	std::string result = channel.label + ": " + display_value(value);
	if (!channel.unit.empty())
	{
		result += ' ' + channel.unit;
	}
	return to_cockpit_font_text(result + '\n');
}

void append_status(std::string& status, const std::string& message)
{
	if (!status.empty())
	{
		status += " | ";
	}
	status += message;
}

void append_hub_status(
	std::string& output,
	const DcsBridge::Internal::DebugTelemetryHubStatus& status)
{
	if (!status.publication_failed)
	{
		return;
	}
	std::ostringstream message;
	message << "[Telemetry Error] channel " << status.channel_id << ": "
		<< status.message.data() << " (count "
		<< status.flight_failure_count << ')';
	append_status(output, message.str());
}

void append_csv_status(
	std::string& output,
	const DcsBridge::Internal::DebugCsvStatus& status)
{
	if (status.ready || status.error_code == 0)
	{
		return;
	}
	std::ostringstream message;
	message << "[debug.csv Error] ";
	if (status.failed_operation != nullptr)
	{
		message << status.failed_operation << ' ';
	}
	message << "code " << status.error_code;
	append_status(output, message.str());
}

void append_overflow_status(std::string& output, std::size_t channel_count)
{
	using DcsBridge::Internal::kDebugIndicatorDataCapacity;
	if (channel_count <= kDebugIndicatorDataCapacity)
	{
		return;
	}
	append_status(
		output,
		"+" + std::to_string(channel_count - kDebugIndicatorDataCapacity) +
		" channels not shown - see debug.csv");
}

DcsBridge::Internal::CockpitParameterEvents event_from(
	const DcsBridge::Internal::CockpitParameterWriteResult& result)
{
	DcsBridge::Internal::CockpitParameterEvents events;
	return result.event ? events.with_event(*result.event) : events;
}
}

namespace DcsBridge
{
namespace Internal
{
DebugIndicatorExporter::DebugIndicatorExporter(
	cockpit_param_api api,
	const DebugTelemetryHub& hub)
	: api_(api),
	hub_(hub),
	visible_parameter_(cockpit_parameter_writer(
		DcsIds::CockpitParams::DebugIndicatorVisible)),
	text_parameters_ {
		cockpit_parameter_writer(DcsIds::CockpitParams::DebugIndicatorText1),
		cockpit_parameter_writer(DcsIds::CockpitParams::DebugIndicatorText2),
		cockpit_parameter_writer(DcsIds::CockpitParams::DebugIndicatorText3),
		cockpit_parameter_writer(DcsIds::CockpitParams::DebugIndicatorText4),
		cockpit_parameter_writer(DcsIds::CockpitParams::DebugIndicatorText5),
		cockpit_parameter_writer(DcsIds::CockpitParams::DebugIndicatorText6),
		cockpit_parameter_writer(DcsIds::CockpitParams::DebugIndicatorStatus)
	}
{
}

CockpitParameterEvents DebugIndicatorExporter::begin_flight()
{
	const std::lock_guard<std::mutex> lock(mutex_);
	visible_parameter_.reset();
	for (CockpitParameterEndpoint& parameter : text_parameters_)
	{
		parameter.reset();
	}
	invalidate_text_cache();
	visible_ = false;
	return clear_text(true).merged(set_visible(false));
}

CockpitParameterEvents DebugIndicatorExporter::release_flight()
{
	const std::lock_guard<std::mutex> lock(mutex_);
	visible_ = false;
	invalidate_text_cache();
	return clear_text(true).merged(set_visible(false));
}

CockpitParameterEvents DebugIndicatorExporter::toggle()
{
	const std::lock_guard<std::mutex> lock(mutex_);
	return set_visible(!visible_);
}

CockpitParameterEvents DebugIndicatorExporter::export_latest(
	const DebugCsvStatus& csv_status)
{
	const std::lock_guard<std::mutex> lock(mutex_);
	if (!visible_)
	{
		return {};
	}
	return write_payload(format_latest(csv_status), false);
}

bool DebugIndicatorExporter::visible() const
{
	const std::lock_guard<std::mutex> lock(mutex_);
	return visible_;
}

CockpitParameterEvents DebugIndicatorExporter::set_visible(bool value)
{
	const CockpitParameterWriteResult result =
		visible_parameter_.write_number(api_, value ? 1.0 : 0.0);
	visible_ = result.success && value;
	return event_from(result);
}

CockpitParameterEvents DebugIndicatorExporter::clear_text(bool force)
{
	return write_payload({}, force);
}

CockpitParameterEvents DebugIndicatorExporter::write_payload(
	const DebugIndicatorPayload& payload,
	bool force)
{
	CockpitParameterEvents events;
	for (std::size_t index = 0; index < payload.blocks.size(); ++index)
	{
		events = events.merged(write_text(index, payload.blocks[index], force));
	}
	return events.merged(
		write_text(kDebugIndicatorStatusIndex, payload.status, force));
}

CockpitParameterEvents DebugIndicatorExporter::write_text(
	std::size_t index,
	const std::string& text,
	bool force)
{
	if (!force && text_cache_valid_[index] && cached_text_[index] == text)
	{
		return {};
	}
	const CockpitParameterWriteResult result =
		text_parameters_[index].write_string(api_, text);
	text_cache_valid_[index] = result.success;
	if (result.success)
	{
		cached_text_[index] = text;
	}
	return event_from(result);
}

DebugIndicatorPayload DebugIndicatorExporter::format_latest(
	const DebugCsvStatus& csv_status) const
{
	const Core::DebugTelemetrySnapshot snapshot = hub_.latest_snapshot();
	if (snapshot.channels.size() != snapshot.values.size())
	{
		throw std::logic_error(
			"Debug Indicator snapshot channel/value counts differ.");
	}
	DebugIndicatorPayload result;
	const std::size_t count =
		(std::min)(snapshot.channels.size(), kDebugIndicatorDataCapacity);
	for (std::size_t index = 0; index < count; ++index)
	{
		result.blocks[index / kDebugIndicatorRowsPerBlock] +=
			format_channel(snapshot.channels[index], snapshot.values[index]);
	}
	append_hub_status(result.status, hub_.status());
	append_csv_status(result.status, csv_status);
	append_overflow_status(result.status, snapshot.channels.size());
	result.status = to_cockpit_font_text(result.status);
	return result;
}

void DebugIndicatorExporter::invalidate_text_cache()
{
	text_cache_valid_.fill(false);
}
}
}
