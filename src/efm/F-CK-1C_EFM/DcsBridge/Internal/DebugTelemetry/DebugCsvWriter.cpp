#include "DebugCsvWriter.h"
#include "DebugTelemetryValueFormat.h"

#include "../../../Common/PathUtils.h"

#include <cerrno>
#include <cmath>
#include <limits>
#include <share.h>
#include <stdexcept>
#include <system_error>

namespace
{
constexpr const char* kDebugCsvFileName = "debug.csv";
constexpr std::chrono::milliseconds kCsvFlushInterval(100);
constexpr std::size_t kErrorMessageCapacity = 1400;
constexpr long double kNanosecondsPerSecond = 1'000'000'000.0L;

int io_error_code()
{
	return errno != 0 ? errno : EIO;
}

void append_csv_cell(std::string& output, const std::string& value)
{
	const bool quote = value.find_first_of(",\"\r\n") != std::string::npos;
	if (!quote)
	{
		output += value;
		return;
	}
	output += '"';
	for (char character : value)
	{
		if (character == '"') output += '"';
		output += character;
	}
	output += '"';
}

void append_csv_value(
	std::string& output,
	const std::optional<Core::DebugTelemetryValue>& value)
{
	output += ',';
	append_csv_cell(
		output,
		value ? DcsBridge::Internal::format_debug_telemetry_value(*value) : "-");
}
}

namespace DcsBridge
{
namespace Internal
{
void LatestDebugBatchMailbox::publish(DebugSampleBatch batch)
{
	{
		std::lock_guard<std::mutex> lock(mutex_);
		pending_ = std::move(batch);
	}
	condition_.notify_one();
}

DebugMailboxRead LatestDebugBatchMailbox::wait_until(
	const std::chrono::steady_clock::time_point& deadline)
{
	std::unique_lock<std::mutex> lock(mutex_);
	const auto ready = [this]()
	{
		return pending_.has_value() || stopping_ ||
			requested_flush_ > completed_flush_;
	};
	if (deadline == std::chrono::steady_clock::time_point::max())
	{
		condition_.wait(lock, ready);
	}
	else
	{
		(void)condition_.wait_until(lock, deadline, ready);
	}
	DebugMailboxRead result = {
		std::move(pending_), requested_flush_, stopping_
	};
	pending_.reset();
	return result;
}

std::uint64_t LatestDebugBatchMailbox::request_flush()
{
	std::lock_guard<std::mutex> lock(mutex_);
	const std::uint64_t request = ++requested_flush_;
	condition_.notify_one();
	return request;
}

void LatestDebugBatchMailbox::wait_for_flush(std::uint64_t request)
{
	std::unique_lock<std::mutex> lock(mutex_);
	condition_.wait(lock, [this, request]()
	{
		return completed_flush_ >= request || stopping_;
	});
}

void LatestDebugBatchMailbox::complete_flush(std::uint64_t request)
{
	{
		std::lock_guard<std::mutex> lock(mutex_);
		completed_flush_ = (std::max)(completed_flush_, request);
	}
	condition_.notify_all();
}

void LatestDebugBatchMailbox::stop()
{
	{
		std::lock_guard<std::mutex> lock(mutex_);
		stopping_ = true;
	}
	condition_.notify_all();
}

std::string format_debug_csv_header(
	const std::vector<Core::DebugTelemetryChannelDescriptor>& schema)
{
	std::string result = "sequence,simulation_time_s";
	for (const auto& channel : schema)
	{
		result += ',';
		append_csv_cell(result, channel.name);
	}
	result += '\n';
	return result;
}

std::string format_debug_csv_row(
	const Core::DebugTelemetrySample& sample)
{
	std::string result = std::to_string(sample.sequence);
	result += ',' + format_debug_telemetry_value(
		Core::DebugTelemetryValue(
			static_cast<double>(sample.simulation_time.count()) /
			static_cast<double>(kNanosecondsPerSecond)));
	for (const auto& value : sample.values)
	{
		append_csv_value(result, value);
	}
	result += '\n';
	return result;
}

Core::DebugSimulationTime debug_time_from_seconds(double simulation_time_s)
{
	if (!std::isfinite(simulation_time_s) || simulation_time_s < 0.0)
	{
		throw std::invalid_argument(
			"Debug CSV simulation time must be finite and non-negative.");
	}
	const long double nanoseconds =
		static_cast<long double>(simulation_time_s) * kNanosecondsPerSecond;
	const long double maximum = static_cast<long double>(
		(std::numeric_limits<std::int64_t>::max)());
	if (nanoseconds > maximum)
	{
		throw std::overflow_error(
			"Debug CSV simulation time exceeded its range.");
	}
	return Core::DebugSimulationTime(
		static_cast<std::int64_t>(std::llround(nanoseconds)));
}

DebugCsvWriter::DebugCsvWriter(
	const char* module_root,
	EventLog& event_log,
	DebugTelemetryHub& hub)
	: event_log_(event_log),
	hub_(hub),
	last_flush_time_(std::chrono::steady_clock::now())
{
	Common::copy_path(module_root_, sizeof(module_root_), module_root);
}

DebugCsvWriter::~DebugCsvWriter()
{
	mailbox_.stop();
	if (worker_.joinable()) worker_.join();
	close_file();
}

void DebugCsvWriter::publish_start(double simulation_time_s)
{
	std::lock_guard<std::mutex> lock(publication_mutex_);
	++current_flight_id_;
	const auto current_schema = hub_.schema();
	if (schema_.empty()) schema_ = current_schema;
	if (!schema_.empty() && schema_.size() != current_schema.size())
	{
		throw std::logic_error("Debug CSV schema changed between flights.");
	}
	report_hub_failure();
	if (schema_.empty())
	{
		(void)prepare_file();
		return;
	}
	if (!ensure_initialized()) return;
	sample_and_publish(simulation_time_s);
}

void DebugCsvWriter::publish_step(double simulation_time_s)
{
	std::lock_guard<std::mutex> lock(publication_mutex_);
	report_hub_failure();
	if (schema_.empty()) return;
	sample_and_publish(simulation_time_s);
}

void DebugCsvWriter::release_flight(double simulation_time_s)
{
	{
		std::lock_guard<std::mutex> lock(publication_mutex_);
		report_hub_failure();
		if (schema_.empty()) return;
		sample_and_publish(simulation_time_s);
	}
	if (!worker_.joinable()) return;
	const std::uint64_t request = mailbox_.request_flush();
	mailbox_.wait_for_flush(request);
}

void DebugCsvWriter::sample_and_publish(double simulation_time_s)
{
	auto samples = hub_.sample_through(
		debug_time_from_seconds(simulation_time_s));
	if (samples.empty()) return;
	mailbox_.publish({ current_flight_id_, std::move(samples) });
}

bool DebugCsvWriter::ensure_initialized()
{
	if (worker_.joinable()) return true;
	if (!prepare_file() || !open_file() || !write_header())
	{
		close_file();
		return false;
	}
	header_written_ = true;
	failed_flight_id_ = 0;
	mark_ready();
	start_worker();
	return worker_.joinable();
}

bool DebugCsvWriter::prepare_file()
{
	if (rotation_complete_) return true;
	const LogFilePreparation location =
		prepare_rotating_log_file(module_root_, kDebugCsvFileName);
	Common::copy_path(active_path_, sizeof(active_path_), location.active_path);
	if (!location.ready)
	{
		report_error({
			Operation::LogFileLifecycle,
			location.error_code,
			std::nullopt,
			current_flight_id_,
			nullptr,
			location.failed_operation });
		return false;
	}
	rotation_complete_ = true;
	return true;
}

bool DebugCsvWriter::open_file()
{
	file_ = _fsopen(active_path_, header_written_ ? "ab" : "wb", _SH_DENYNO);
	if (file_ != nullptr) return true;
	report_error({
		Operation::Open, io_error_code(), std::nullopt, current_flight_id_ });
	return false;
}

bool DebugCsvWriter::write_header()
{
	if (header_written_) return true;
	const std::string header = format_debug_csv_header(schema_);
	if (fwrite(header.data(), 1, header.size(), file_) == header.size() &&
		fflush(file_) == 0)
	{
		return true;
	}
	report_error({
		Operation::WriteHeader,
		io_error_code(),
		std::nullopt,
		current_flight_id_ });
	return false;
}

void DebugCsvWriter::start_worker()
{
	try
	{
		worker_ = std::thread(&DebugCsvWriter::worker_loop, this);
	}
	catch (const std::system_error& error)
	{
		const int code = error.code().value() != 0
			? error.code().value() : EAGAIN;
		report_error({
			Operation::StartWorker, code, std::nullopt, current_flight_id_ });
		close_file();
	}
}

void DebugCsvWriter::worker_loop()
{
	while (true)
	{
		const auto deadline = dirty_
			? last_flush_time_ + kCsvFlushInterval
			: std::chrono::steady_clock::time_point::max();
		const DebugMailboxRead incoming = mailbox_.wait_until(deadline);
		if (incoming.batch) process_batch(*incoming.batch);
		flush_if_due(incoming.stopping || incoming.flush_request != 0);
		if (incoming.flush_request != 0)
		{
			mailbox_.complete_flush(incoming.flush_request);
		}
		if (incoming.stopping) return;
	}
}

void DebugCsvWriter::process_batch(const DebugSampleBatch& batch)
{
	if (file_ == nullptr && failed_flight_id_ == batch.flight_id) return;
	if (file_ == nullptr && !retry_file(batch)) return;
	for (const Core::DebugTelemetrySample& sample : batch.samples)
	{
		write_sample(sample, batch.flight_id);
		if (file_ == nullptr) return;
	}
}

bool DebugCsvWriter::retry_file(const DebugSampleBatch& batch)
{
	if (!prepare_file() || !open_file() || !write_header())
	{
		failed_flight_id_ = batch.flight_id;
		return false;
	}
	failed_flight_id_ = 0;
	mark_ready();
	return true;
}

void DebugCsvWriter::write_sample(
	const Core::DebugTelemetrySample& sample,
	std::uint64_t flight_id)
{
	if (sample.values.size() != schema_.size())
	{
		fail_current_flight({
			Operation::FormatRow,
			EINVAL,
			static_cast<double>(sample.simulation_time.count()) /
				static_cast<double>(kNanosecondsPerSecond),
			flight_id });
		return;
	}
	std::string row;
	try
	{
		row = format_debug_csv_row(sample);
	}
	catch (const std::exception& error)
	{
		fail_current_flight({
			Operation::FormatRow,
			EINVAL,
			static_cast<double>(sample.simulation_time.count()) /
				static_cast<double>(kNanosecondsPerSecond),
			flight_id,
			error.what() });
		return;
	}
	if (fwrite(row.data(), 1, row.size(), file_) != row.size())
	{
		fail_current_flight({
			Operation::WriteRow,
			io_error_code(),
			static_cast<double>(sample.simulation_time.count()) /
				static_cast<double>(kNanosecondsPerSecond),
			flight_id });
		return;
	}
	dirty_ = true;
	last_written_flight_id_ = flight_id;
	last_written_sample_ = sample;
}

void DebugCsvWriter::flush_if_due(bool force)
{
	if (!dirty_ || file_ == nullptr) return;
	const auto now = std::chrono::steady_clock::now();
	if (!force && now < last_flush_time_ + kCsvFlushInterval) return;
	if (fflush(file_) != 0)
	{
		fail_current_flight({
			Operation::Flush,
			io_error_code(),
			static_cast<double>(last_written_sample_.simulation_time.count()) /
				static_cast<double>(kNanosecondsPerSecond),
			last_written_flight_id_ });
		return;
	}
	dirty_ = false;
	last_flush_time_ = now;
}

void DebugCsvWriter::fail_current_flight(const Failure& failure)
{
	failed_flight_id_ = failure.flight_id;
	dirty_ = false;
	close_file();
	report_error(failure);
}

void DebugCsvWriter::report_error(const Failure& failure)
{
	{
		std::lock_guard<std::mutex> lock(status_mutex_);
		status_ = {
			false,
			failure.error_code,
			failure.operation,
			failure.lifecycle_operation };
	}
	char message[kErrorMessageCapacity];
	const char* detail = failure.detail == nullptr ? "" : failure.detail;
	snprintf(
		message,
		sizeof(message),
		"debug_csv operation=%s path=%s os_error=%d detail=%s",
		operation_name(
			failure.operation,
			failure.lifecycle_operation),
		active_path_[0] == '\0' ? "<unresolved>" : active_path_,
		failure.error_code,
		detail);
	(void)event_log_.write({
		EventLevel::Error, failure.simulation_time_s, message });
}

void DebugCsvWriter::mark_ready()
{
	std::lock_guard<std::mutex> lock(status_mutex_);
	status_ = {
		true, 0, Operation::None, LogFileOperation::None
	};
}

void DebugCsvWriter::report_hub_failure()
{
	const DebugTelemetryHubStatus status = hub_.status();
	if (!status.publication_failed ||
		status.failure_sequence == last_hub_failure_sequence_)
	{
		return;
	}
	last_hub_failure_sequence_ = status.failure_sequence;
	char message[kErrorMessageCapacity];
	snprintf(
		message,
		sizeof(message),
		"debug_telemetry operation=publish channel=%u time_ns=%lld "
		"count=%llu detail=%s",
		static_cast<unsigned int>(status.channel_id),
		static_cast<long long>(status.simulation_time_ns),
		static_cast<unsigned long long>(status.flight_failure_count),
		status.message.data());
	(void)event_log_.write({ EventLevel::Error, std::nullopt, message });
}

void DebugCsvWriter::close_file()
{
	if (file_ == nullptr) return;
	fclose(file_);
	file_ = nullptr;
}

DebugCsvStatus DebugCsvWriter::status() const
{
	std::lock_guard<std::mutex> lock(status_mutex_);
	return {
		status_.ready,
		status_.error_code,
		status_.failed_operation == Operation::None
			? nullptr : operation_name(
				status_.failed_operation,
				status_.lifecycle_operation)
	};
}

bool DebugCsvWriter::is_ready() const
{
	return status().ready;
}

const char* DebugCsvWriter::operation_name(
	Operation operation,
	LogFileOperation lifecycle_operation)
{
	if (operation == Operation::LogFileLifecycle)
	{
		if (lifecycle_operation == LogFileOperation::None)
		{
			throw std::logic_error(
				"Log file lifecycle failure omitted its operation.");
		}
		return log_file_operation_name(lifecycle_operation);
	}
	switch (operation)
	{
	case Operation::None: return "none";
	case Operation::LogFileLifecycle: break;
	case Operation::Open: return "open";
	case Operation::WriteHeader: return "write_header";
	case Operation::StartWorker: return "start_worker";
	case Operation::FormatRow: return "format_row";
	case Operation::WriteRow: return "write_row";
	case Operation::Flush: return "flush";
	}
	return "unknown";
}
}
}
