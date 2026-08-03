#include "TestHarness.h"
#include "TestFileUtils.h"

#include "DcsBridge/Internal/DebugTelemetry/DebugCsvWriter.h"

#include <chrono>
#include <filesystem>
#include <limits>
#include <string>

namespace
{
using DcsBridge::Internal::DebugCsvWriter;
using DcsBridge::Internal::DebugTelemetryHub;
using namespace std::chrono_literals;
constexpr auto kIsolatedPublishErrors =
	Core::DebugTelemetryPublishErrorPolicy::IsolateAndReport;

Core::DebugTelemetryChannelDescriptor descriptor(const char* name)
{
	return {
		name,
		"Value",
		Core::DebugTelemetryValueType::Double,
		"",
		"csv test"
	};
}

void test_csv_formatting_and_escaping(Tests::Context& context)
{
	auto text_descriptor = descriptor("text,value");
	text_descriptor.value_type = Core::DebugTelemetryValueType::Text;
	const std::string header =
		DcsBridge::Internal::format_debug_csv_header({ text_descriptor });
	TEST_EXPECT(context,
		header == "sequence,simulation_time_s,\"text,value\"\n");
	Core::DebugTelemetrySample sample;
	sample.sequence = 2;
	sample.simulation_time = 15'625'000ns;
	sample.values = { std::string("alpha,\"beta\"\nline") };
	const std::string row =
		DcsBridge::Internal::format_debug_csv_row(sample);
	TEST_EXPECT(context,
		row == "2,0.015625,\"alpha,\"\"beta\"\"\nline\"\n");
}

void test_csv_formats_all_values_and_nonfinite_numbers(
	Tests::Context& context)
{
	Core::DebugTelemetrySample sample;
	sample.sequence = 3;
	sample.simulation_time = 1s;
	sample.values = {
		std::nullopt,
		std::int64_t(-7),
		true,
		1.2345678901234567,
		(std::numeric_limits<double>::quiet_NaN)(),
		(std::numeric_limits<double>::infinity)(),
		-(std::numeric_limits<double>::infinity)()
	};
	TEST_EXPECT(context, DcsBridge::Internal::format_debug_csv_row(sample) ==
		"3,1,-,-7,True,1.2345678901234567,nan,inf,-inf\n");
}

void test_file_lifecycle_and_resampling(Tests::Context& context)
{
	TestFiles::TemporaryDirectory root("debug_csv");
	TEST_EXPECT(context, root.valid());
	const auto log = root.path() / "log";
	std::filesystem::create_directories(log);
	const auto active = log / "debug.csv";
	const auto old = log / "debug.csv.old";
	TestFiles::write_text(active, "previous\n");
	TestFiles::write_text(old, "stale\n");
	DcsBridge::Internal::EventLog event_log(root.path().string().c_str());
	DebugTelemetryHub hub(kIsolatedPublishErrors);
	hub.begin_flight();
	auto value = hub.declare_channel<double>(descriptor("value"));
	value.publish(0ns, 1.0);
	hub.seal_schema();
	{
		DebugCsvWriter writer(root.path().string().c_str(), event_log, hub);
		writer.publish_start(0.0);
		value.publish(10ms, 2.0);
		writer.publish_step(0.010);
		value.publish(20ms, 3.0);
		writer.publish_step(0.040);
		writer.release_flight(0.040);
		TEST_EXPECT(context, writer.is_ready());
		TEST_EXPECT(context,
			TestFiles::read_text_while_open(old) == "previous\n");
		const std::string shared_output =
			TestFiles::read_text_while_open(active);
		TEST_EXPECT(context, shared_output.find(
			"sequence,simulation_time_s,value\n") == 0);
	}
	TEST_EXPECT(context, TestFiles::read_text_while_open(old) == "previous\n");
	const std::string output = TestFiles::read_text_while_open(active);
	TEST_EXPECT(context, output.find(
		"sequence,simulation_time_s,value\n") == 0);
	// The latest-batch mailbox may replace the initial batch before the worker
	// consumes it. Sequence gaps deliberately expose that overload behavior.
	TEST_EXPECT(context, output.find("1,0.015625,2\n") != std::string::npos);
	TEST_EXPECT(context, output.find("2,0.03125,3\n") != std::string::npos);
}

void test_no_channels_creates_no_file(Tests::Context& context)
{
	TestFiles::TemporaryDirectory root("debug_empty");
	TEST_EXPECT(context, root.valid());
	DcsBridge::Internal::EventLog event_log(root.path().string().c_str());
	DebugTelemetryHub hub(kIsolatedPublishErrors);
	hub.begin_flight();
	hub.seal_schema();
	{
		DebugCsvWriter writer(root.path().string().c_str(), event_log, hub);
		writer.publish_start(0.0);
		writer.publish_step(0.1);
		writer.release_flight(0.1);
	}
	TEST_EXPECT(context,
		!std::filesystem::exists(root.path() / "log" / "debug.csv"));
}

void test_no_channels_still_rotates_an_existing_file(
	Tests::Context& context)
{
	TestFiles::TemporaryDirectory root("debug_empty_rotation");
	TEST_EXPECT(context, root.valid());
	const auto log = root.path() / "log";
	std::filesystem::create_directories(log);
	TestFiles::write_text(log / "debug.csv", "previous\n");
	TestFiles::write_text(log / "debug.csv.old", "stale\n");
	DcsBridge::Internal::EventLog event_log(root.path().string().c_str());
	DebugTelemetryHub hub(kIsolatedPublishErrors);
	hub.begin_flight();
	hub.seal_schema();
	DebugCsvWriter writer(root.path().string().c_str(), event_log, hub);
	writer.publish_start(0.0);
	TEST_EXPECT(context,
		!std::filesystem::exists(log / "debug.csv"));
	TEST_EXPECT(context,
		TestFiles::read_text_while_open(log / "debug.csv.old") ==
			"previous\n");
}

void test_file_error_is_explicit(Tests::Context& context)
{
	TestFiles::TemporaryDirectory root("debug_error");
	TEST_EXPECT(context, root.valid());
	DcsBridge::Internal::EventLog event_log(root.path().string().c_str());
	DebugTelemetryHub hub(kIsolatedPublishErrors);
	hub.begin_flight();
	(void)hub.declare_channel<double>(descriptor("value"));
	hub.seal_schema();
	DebugCsvWriter writer("", event_log, hub);
	writer.publish_start(0.0);
	const auto status = writer.status();
	TEST_EXPECT(context, !status.ready);
	TEST_EXPECT(context, status.error_code != 0);
	TEST_EXPECT(context, status.failed_operation != nullptr);
}

void test_next_flight_retries_a_transient_path_error(Tests::Context& context)
{
	TestFiles::TemporaryDirectory root("debug_retry");
	TEST_EXPECT(context, root.valid());
	const auto blocked = root.path() / "module";
	TestFiles::write_text(blocked, "temporarily not a directory");
	DcsBridge::Internal::EventLog event_log(root.path().string().c_str());
	DebugTelemetryHub hub(kIsolatedPublishErrors);
	hub.begin_flight();
	(void)hub.declare_channel<double>(descriptor("value"));
	hub.seal_schema();
	DebugCsvWriter writer(blocked.string().c_str(), event_log, hub);
	writer.publish_start(0.0);
	TEST_EXPECT(context, !writer.status().ready);
	hub.release_flight();
	std::filesystem::remove(blocked);
	std::filesystem::create_directory(blocked);
	hub.begin_flight();
	(void)hub.declare_channel<double>(descriptor("value"));
	hub.seal_schema();
	writer.publish_start(0.0);
	writer.release_flight(0.0);
	TEST_EXPECT(context, writer.status().ready);
	TEST_EXPECT(context, writer.status().error_code == 0);
	TEST_EXPECT(context,
		std::filesystem::exists(blocked / "log" / "debug.csv"));
}
}

void run_debug_csv_writer_tests(Tests::Context& context)
{
	test_csv_formatting_and_escaping(context);
	test_csv_formats_all_values_and_nonfinite_numbers(context);
	test_file_lifecycle_and_resampling(context);
	test_no_channels_creates_no_file(context);
	test_no_channels_still_rotates_an_existing_file(context);
	test_file_error_is_explicit(context);
	test_next_flight_retries_a_transient_path_error(context);
}
