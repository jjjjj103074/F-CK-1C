#pragma once

#include <cstdio>
#include <cstdint>
#include <mutex>
#include <optional>
#include <vector>

namespace DcsBridge
{
namespace Internal
{
enum class EventLevel
{
	Info,
	Warning,
	Error
};

enum class CountedWarningKind
{
	UnknownCommand,
	UnknownParam
};

struct EventRecord
{
	EventLevel level = EventLevel::Info;
	std::optional<double> simulation_time_s;
	const char* message = nullptr;
};

struct CountedWarningEvent
{
	CountedWarningKind kind = CountedWarningKind::UnknownCommand;
	std::int64_t id = 0;
	std::optional<double> simulation_time_s;
	const char* first_message = nullptr;
};

class EventLog final
{
public:
	explicit EventLog(const char* module_root);
	~EventLog();

	EventLog(const EventLog&) = delete;
	EventLog& operator=(const EventLog&) = delete;

	bool write(const EventRecord& record);
	bool write_counted_warning(const CountedWarningEvent& event);
	bool release_flight(const std::optional<double>& simulation_time_s);
	bool is_open() const;
	int last_error_code() const;

private:
	struct WarningCounter
	{
		CountedWarningKind kind;
		std::int64_t id;
		std::uint64_t count;
	};

	bool initialize(const char* module_root);
	bool format_line(
		const EventRecord& record,
		char* output,
		size_t capacity) const;
	bool write_unlocked(const EventRecord& record);
	bool write_summary(
		const WarningCounter& counter,
		const std::optional<double>& simulation_time_s);
	WarningCounter* find_counter(
		CountedWarningKind kind,
		std::int64_t id);
	void set_error(int error_code);

	mutable std::mutex mutex_;
	FILE* file_ = nullptr;
	int last_error_code_ = 0;
	std::vector<WarningCounter> warning_counters_;
};
}
}
