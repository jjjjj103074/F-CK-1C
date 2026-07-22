#pragma once

#include "EventLog.h"
#include "LogFileLifecycle.h"
#include "../../Core/FrameContracts.h"

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <optional>
#include <thread>

namespace DcsBridge
{
namespace Internal
{
inline constexpr std::size_t kStateCsvRowCapacity = 8192;

struct TelemetryRecord
{
	std::uint64_t publish_version = 0;
	std::uint64_t flight_id = 0;
	std::uint64_t sequence = 0;
	Core::FrameOutput output;
};

struct MailboxRead
{
	std::optional<TelemetryRecord> record;
	bool stopping = false;
};

class LatestTelemetryMailbox final
{
public:
	void publish(
		std::uint64_t flight_id,
		std::uint64_t sequence,
		const Core::FrameOutput& output);
	std::optional<TelemetryRecord> take_latest();
	MailboxRead wait_until(
		const std::chrono::steady_clock::time_point& deadline);
	void stop();

private:
	std::mutex mutex_;
	std::condition_variable condition_;
	std::optional<TelemetryRecord> pending_;
	std::uint64_t next_publish_version_ = 1;
	bool stopping_ = false;
};

struct FormattedStateCsvRow
{
	std::array<char, kStateCsvRowCapacity> data = {};
	std::size_t size = 0;
	bool valid = false;
};

const char* state_csv_header();
std::size_t state_csv_header_size();
FormattedStateCsvRow format_state_csv_row(const TelemetryRecord& record);

class StateCsvWriter final
{
public:
	StateCsvWriter(const char* module_root, EventLog& event_log);
	~StateCsvWriter();

	StateCsvWriter(const StateCsvWriter&) = delete;
	StateCsvWriter& operator=(const StateCsvWriter&) = delete;

	void publish_start(const Core::FrameOutput& output);
	void publish_step(const Core::FrameOutput& output);
	bool is_ready() const;
	int last_error_code() const;

private:
	struct Failure
	{
		const char* operation;
		int error_code;
		std::optional<double> simulation_time_s;
		std::uint64_t flight_id;
	};

	void initialize_execution_file();
	void start_worker();
	void worker_loop();
	void process_record(const TelemetryRecord& record);
	bool retry_file(const TelemetryRecord& record);
	bool write_header();
	void flush_if_due(bool force);
	void fail_current_flight(const Failure& failure);
	void report_error(const Failure& failure);
	void close_file();

	EventLog& event_log_;
	char module_root_[kLogFilePathCapacity] = {};
	char active_path_[kLogFilePathCapacity] = {};
	LatestTelemetryMailbox mailbox_;
	std::mutex publication_mutex_;
	std::uint64_t current_flight_id_ = 0;
	std::uint64_t next_sequence_ = 0;
	std::thread worker_;
	FILE* file_ = nullptr;
	bool rotation_complete_ = false;
	bool header_written_ = false;
	bool dirty_ = false;
	std::uint64_t failed_flight_id_ = 0;
	TelemetryRecord last_written_record_;
	std::chrono::steady_clock::time_point last_flush_time_;
	std::atomic<bool> ready_ = false;
	std::atomic<int> last_error_code_ = 0;
};
}
}
