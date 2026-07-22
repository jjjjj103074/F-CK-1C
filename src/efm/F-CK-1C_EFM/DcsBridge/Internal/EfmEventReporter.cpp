#include "EfmEventReporter.h"

#include "../../Diagnostics/RuntimeDiagnostics.h"

#include <cstdio>

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

void EfmEventReporter::log_start(
	Core::StartMode mode,
	double simulation_time_s)
{
	char message[kEventMessageCapacity];
	snprintf(message, sizeof(message), "flight start mode=%s", start_mode_name(mode));
	write(EventLevel::Info, simulation_time_s, message);
}

void EfmEventReporter::log_damage(
	const Core::Fck1cEfmSnapshot& snapshot,
	int element,
	double integrity)
{
	char message[kEventMessageCapacity];
	Diagnostics::format_damage_event(
		{ message, sizeof(message) },
		{ element, integrity, snapshot.gameplay.invincible });
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
}
}
}
