#pragma once

#include "DebugTelemetryHub.h"
#include "DebugTelemetryStatus.h"
#include "../EventLog.h"
#include "../LogFileLifecycle.h"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace DcsBridge
{
namespace Internal
{
struct DebugSampleBatch
{
	std::uint64_t flight_id = 0;
	std::vector<Core::DebugTelemetrySample> samples;
};

struct DebugMailboxRead
{
	std::optional<DebugSampleBatch> batch;
	std::uint64_t flush_request = 0;
	bool stopping = false;
};

class LatestDebugBatchMailbox final
{
public:
	void publish(DebugSampleBatch batch);
	DebugMailboxRead wait_until(
		const std::chrono::steady_clock::time_point& deadline);
	std::uint64_t request_flush();
	void wait_for_flush(std::uint64_t request);
	void complete_flush(std::uint64_t request);
	void stop();

private:
	std::mutex mutex_;
	std::condition_variable condition_;
	std::optional<DebugSampleBatch> pending_;
	std::uint64_t requested_flush_ = 0;
	std::uint64_t completed_flush_ = 0;
	bool stopping_ = false;
};

std::string format_debug_csv_header(
	const std::vector<Core::DebugTelemetryChannelDescriptor>& schema);
std::string format_debug_csv_row(
	const Core::DebugTelemetrySample& sample);
Core::DebugSimulationTime debug_time_from_seconds(double simulation_time_s);

class DebugCsvWriter final
{
public:
	DebugCsvWriter(
		const char* module_root,
		EventLog& event_log,
		DebugTelemetryHub& hub);
	~DebugCsvWriter();

	DebugCsvWriter(const DebugCsvWriter&) = delete;
	DebugCsvWriter& operator=(const DebugCsvWriter&) = delete;

	void publish_start(double simulation_time_s);
	void publish_step(double simulation_time_s);
	void release_flight(double simulation_time_s);
	DebugCsvStatus status() const;
	bool is_ready() const;

private:
	enum class Operation
	{
		None,
		LogFileLifecycle,
		Open,
		WriteHeader,
		StartWorker,
		FormatRow,
		WriteRow,
		Flush
	};

	struct Failure
	{
		Operation operation = Operation::None;
		int error_code = 0;
		std::optional<double> simulation_time_s;
		std::uint64_t flight_id = 0;
		const char* detail = nullptr;
		LogFileOperation lifecycle_operation = LogFileOperation::None;
	};
	struct StatusState
	{
		bool ready = false;
		int error_code = 0;
		Operation failed_operation = Operation::None;
		LogFileOperation lifecycle_operation = LogFileOperation::None;
	};

	bool ensure_initialized();
	bool prepare_file();
	bool open_file();
	bool write_header();
	void start_worker();
	void worker_loop();
	void process_batch(const DebugSampleBatch& batch);
	bool retry_file(const DebugSampleBatch& batch);
	void write_sample(
		const Core::DebugTelemetrySample& sample,
		std::uint64_t flight_id);
	void flush_if_due(bool force);
	void fail_current_flight(const Failure& failure);
	void report_error(const Failure& failure);
	void report_hub_failure();
	void mark_ready();
	void close_file();
	void sample_and_publish(double simulation_time_s);
	static const char* operation_name(
		Operation operation,
		LogFileOperation lifecycle_operation);

	EventLog& event_log_;
	DebugTelemetryHub& hub_;
	char module_root_[kLogFilePathCapacity] = {};
	char active_path_[kLogFilePathCapacity] = {};
	std::vector<Core::DebugTelemetryChannelDescriptor> schema_;
	LatestDebugBatchMailbox mailbox_;
	std::mutex publication_mutex_;
	std::uint64_t current_flight_id_ = 0;
	std::thread worker_;
	FILE* file_ = nullptr;
	bool rotation_complete_ = false;
	bool header_written_ = false;
	bool dirty_ = false;
	std::uint64_t failed_flight_id_ = 0;
	std::uint64_t last_written_flight_id_ = 0;
	std::uint64_t last_hub_failure_sequence_ = 0;
	Core::DebugTelemetrySample last_written_sample_;
	std::chrono::steady_clock::time_point last_flush_time_;
	mutable std::mutex status_mutex_;
	StatusState status_;
};
}
}
