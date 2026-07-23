#include "EfmEventReporter.h"

#include "RuntimeDiagnostics.h"

#include <cstdio>
#include <limits>

namespace
{
constexpr size_t kEventMessageCapacity = 1200;

const char* start_mode_name(Core::StartMode mode)
{
	switch (mode)
	{
	case Core::StartMode::ColdGround:
		return "cold_ground";
	case Core::StartMode::HotGround:
		return "hot_ground";
	case Core::StartMode::HotAir:
		return "hot_air";
	}
	return "unknown";
}
}

namespace DcsBridge
{
namespace Internal
{
EfmEventReporter::EfmEventReporter(
	EventLog& event_log,
	const OutputStore& output_store)
	: event_log_(event_log),
	output_store_(output_store)
{
}

std::optional<double> EfmEventReporter::latest_simulation_time() const
{
	const std::optional<Core::FrameOutput> output = output_store_.read();
	return output
		? std::optional<double>(output->simulation_time_s)
		: std::nullopt;
}

void EfmEventReporter::write(
	EventLevel level,
	const std::optional<double>& simulation_time_s,
	const char* message)
{
	(void)event_log_.write({ level, simulation_time_s, message });
}

void EfmEventReporter::log_callback_lifecycle_error(
	const CallbackContext& context,
	const char* state)
{
	char message[kEventMessageCapacity];
	Diagnostics::format_callback_lifecycle_error(
		{ message, sizeof(message) },
		{
			context.name,
			state,
			context.parameter_name,
			context.parameter_value
		});
	write(EventLevel::Error, latest_simulation_time(), message);
}

void EfmEventReporter::log_unavailable_output(const CallbackContext& context)
{
	log_callback_lifecycle_error(
		context,
		output_store_.is_released() ? "released" : "before_start");
}

void EfmEventReporter::log_invalid_frame_dt(double dt_s)
{
	char message[kEventMessageCapacity];
	Diagnostics::format_invalid_frame_dt_error(
		{ message, sizeof(message) },
		{ dt_s });
	write(EventLevel::Error, latest_simulation_time(), message);
}

void EfmEventReporter::log_invalid_numeric(
	const char* callback,
	const char* field,
	double value)
{
	constexpr int kRoundTripDoubleDigits = std::numeric_limits<double>::max_digits10;
	char message[kEventMessageCapacity];
	snprintf(
		message,
		sizeof(message),
		"callback=%s field=%s invalid numeric value=%.*g",
		callback,
		field,
		kRoundTripDoubleDigits,
		value);
	write(EventLevel::Error, latest_simulation_time(), message);
}

void EfmEventReporter::log_unknown_command(int command, float value)
{
	constexpr int kRoundTripFloatDigits = std::numeric_limits<float>::max_digits10;
	char message[kEventMessageCapacity];
	snprintf(
		message,
		sizeof(message),
		"callback=ed_fm_set_command command=%d unknown value=%.*g",
		command,
		kRoundTripFloatDigits,
		static_cast<double>(value));
	(void)event_log_.write_counted_warning({
		CountedWarningKind::UnknownCommand,
		command,
		latest_simulation_time(),
		message
	});
}

void EfmEventReporter::log_command_binding_error(
	const char* reason,
	int command)
{
	char message[kEventMessageCapacity];
	snprintf(
		message,
		sizeof(message),
		"callback=ed_fm_set_command binding_table_error=%s command=%d",
		reason,
		command);
	write(EventLevel::Error, latest_simulation_time(), message);
}

void EfmEventReporter::log_missing_param(unsigned index)
{
	char message[kEventMessageCapacity];
	snprintf(
		message,
		sizeof(message),
		"callback=ed_fm_get_param index=%u missing mapping",
		index);
	(void)event_log_.write_counted_warning({
		CountedWarningKind::UnknownParam,
		index,
		latest_simulation_time(),
		message
	});
}

void EfmEventReporter::log_missing_param_data(
	unsigned index,
	const char* category)
{
	char message[kEventMessageCapacity];
	snprintf(
		message,
		sizeof(message),
		"callback=ed_fm_get_param index=%u missing data=%s",
		index,
		category);
	write(EventLevel::Error, latest_simulation_time(), message);
}

void EfmEventReporter::log_invalid_index(
	const char* callback,
	int index)
{
	char message[kEventMessageCapacity];
	snprintf(
		message,
		sizeof(message),
		"callback=%s invalid index=%d",
		callback,
		index);
	write(EventLevel::Error, latest_simulation_time(), message);
}

void EfmEventReporter::log_draw_args_buffer_error(
	bool null_pointer,
	std::size_t size,
	std::size_t required)
{
	char message[kEventMessageCapacity];
	snprintf(
		message,
		sizeof(message),
		"callback=ed_fm_set_draw_args pointer_null=%s size=%zu required=%zu",
		null_pointer ? "true" : "false",
		size,
		required);
	write(EventLevel::Error, latest_simulation_time(), message);
}

void EfmEventReporter::log_cockpit_parameter_events(
	const CockpitParameterEvents& events)
{
	constexpr int kRoundTripDoubleDigits = std::numeric_limits<double>::max_digits10;
	for (std::size_t index = 0; index < events.count; ++index)
	{
		const CockpitParameterEvent& event = events.items[index];
		char message[kEventMessageCapacity];
		if (event.type == CockpitParameterEventType::Recovery)
		{
			snprintf(
				message,
				sizeof(message),
				"cockpit parameter=%s recovered",
				event.parameter_name);
			write(EventLevel::Info, latest_simulation_time(), message);
			continue;
		}
		if (event.has_value)
		{
			snprintf(
				message,
				sizeof(message),
				"cockpit parameter=%s unavailable reason=%s value=%.*g",
				event.parameter_name,
				event.reason,
				kRoundTripDoubleDigits,
				event.value);
		}
		else
		{
			snprintf(
				message,
				sizeof(message),
				"cockpit parameter=%s unavailable reason=%s",
				event.parameter_name,
				event.reason);
		}
		write(EventLevel::Error, latest_simulation_time(), message);
	}
}

void EfmEventReporter::log_start(
	Core::StartMode mode,
	double simulation_time_s)
{
	char message[kEventMessageCapacity];
	snprintf(message, sizeof(message), "flight start mode=%s", start_mode_name(mode));
	write(EventLevel::Info, simulation_time_s, message);
}

void EfmEventReporter::log_damage(
	const Core::DamageApplyResult& result,
	int element,
	double integrity)
{
	char message[kEventMessageCapacity];
	Diagnostics::format_damage_event(
		{ message, sizeof(message) },
		{ element, integrity, result.invincible });
	write(EventLevel::Info, latest_simulation_time(), message);
}

void EfmEventReporter::log_suspension_feedback_error(
	int index,
	bool null_info)
{
	char message[kEventMessageCapacity];
	Diagnostics::format_suspension_feedback_error(
		{ message, sizeof(message) },
		{ index, null_info });
	write(EventLevel::Error, latest_simulation_time(), message);
}

void EfmEventReporter::log_configure(const char* config_path)
{
	char message[kEventMessageCapacity];
	snprintf(
		message,
		sizeof(message),
		"module configure config_path=%s",
		config_path ? config_path : "<null>");
	write(EventLevel::Info, std::nullopt, message);
}

void EfmEventReporter::log_release(
	const std::optional<double>& simulation_time_s)
{
	write(EventLevel::Info, simulation_time_s, "flight release");
	(void)event_log_.release_flight(simulation_time_s);
}
}
}
