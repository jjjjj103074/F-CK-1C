#include "EventLog.h"

#include "LogFileLifecycle.h"

#include <cerrno>
#include <cstring>
#include <limits>
#include <share.h>
#include <windows.h>

namespace
{
constexpr size_t kLineCapacity = 2048;
constexpr int kRoundTripDoubleDigits = std::numeric_limits<double>::max_digits10;
constexpr const char* kActiveFileName = "fck1c_efm.log";

const char* level_name(DcsBridge::Internal::EventLevel level)
{
	switch (level)
	{
	case DcsBridge::Internal::EventLevel::Info:
		return "INFO";
	case DcsBridge::Internal::EventLevel::Error:
		return "ERROR";
	}
	return "ERROR";
}

bool format_simulation_time(
	const std::optional<double>& simulation_time_s,
	char* output,
	size_t capacity)
{
	const int length = simulation_time_s
		? snprintf(output, capacity, "%.*g", kRoundTripDoubleDigits, *simulation_time_s)
		: snprintf(output, capacity, "-");
	return length >= 0 && static_cast<size_t>(length) < capacity;
}
}

namespace DcsBridge
{
namespace Internal
{
EventLog::EventLog(const char* module_root)
{
	(void)initialize(module_root);
}

EventLog::~EventLog()
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (file_ != nullptr)
	{
		fclose(file_);
		file_ = nullptr;
	}
}

bool EventLog::initialize(const char* module_root)
{
	const LogFilePreparation location =
		prepare_rotating_log_file(module_root, kActiveFileName);
	if (!location.ready)
	{
		set_error(location.error_code);
		return false;
	}
	file_ = _fsopen(location.active_path, "wb", _SH_DENYNO);
	if (file_ == nullptr)
	{
		set_error(errno);
		return false;
	}
	return true;
}
bool EventLog::format_line(
	const EventRecord& record,
	char* output,
	size_t capacity) const
{
	if (record.message == nullptr)
	{
		return false;
	}
	SYSTEMTIME wall_clock = {};
	GetLocalTime(&wall_clock);
	char simulation_time[64];
	if (!format_simulation_time(
		record.simulation_time_s,
		simulation_time,
		sizeof(simulation_time)))
	{
		return false;
	}
	const int length = snprintf(
		output,
		capacity,
		"[%04u-%02u-%02u %02u:%02u:%02u.%03u][%s][%s] %s\n",
		wall_clock.wYear,
		wall_clock.wMonth,
		wall_clock.wDay,
		wall_clock.wHour,
		wall_clock.wMinute,
		wall_clock.wSecond,
		wall_clock.wMilliseconds,
		simulation_time,
		level_name(record.level),
		record.message);
	return length >= 0 && static_cast<size_t>(length) < capacity;
}

bool EventLog::write(const EventRecord& record)
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (file_ == nullptr)
	{
		return false;
	}
	char line[kLineCapacity];
	if (!format_line(record, line, sizeof(line)))
	{
		set_error(EINVAL);
		return false;
	}
	const size_t length = strlen(line);
	if (fwrite(line, 1, length, file_) != length)
	{
		set_error(errno != 0 ? errno : EIO);
		return false;
	}
	if (fflush(file_) != 0)
	{
		set_error(errno != 0 ? errno : EIO);
		return false;
	}
	return true;
}

bool EventLog::is_open() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return file_ != nullptr;
}

int EventLog::last_error_code() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return last_error_code_;
}

void EventLog::set_error(int error_code)
{
	last_error_code_ = error_code;
}
}
}
