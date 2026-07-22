#pragma once

#include <cstdio>
#include <mutex>
#include <optional>

namespace DcsBridge
{
namespace Internal
{
enum class EventLevel
{
	Info,
	Error
};

struct EventRecord
{
	EventLevel level = EventLevel::Info;
	std::optional<double> simulation_time_s;
	const char* message = nullptr;
};

class EventLog final
{
public:
	explicit EventLog(const char* module_root);
	~EventLog();

	EventLog(const EventLog&) = delete;
	EventLog& operator=(const EventLog&) = delete;

	bool write(const EventRecord& record);
	bool is_open() const;
	int last_error_code() const;

private:
	bool initialize(const char* module_root);
	bool format_line(
		const EventRecord& record,
		char* output,
		size_t capacity) const;
	void set_error(int error_code);

	mutable std::mutex mutex_;
	FILE* file_ = nullptr;
	int last_error_code_ = 0;
};
}
}
