#include "TestHarness.h"
#include "TestFileUtils.h"

#include "DcsBridge/Internal/EfmEventReporter.h"
#include "DcsBridge/Internal/EventLog.h"
#include "DcsBridge/Internal/RuntimeDiagnostics.h"

#include <atomic>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

namespace
{
constexpr int kUnknownCommandId = 404;
constexpr unsigned kMissingParamId = 999;
constexpr float kUnknownCommandValue = 0.5F;
constexpr int kWritesPerThread = 40;
constexpr int kWriterThreadCount = 2;

size_t count_occurrences(const std::string& text, const std::string& value)
{
	size_t count = 0;
	size_t offset = 0;
	while ((offset = text.find(value, offset)) != std::string::npos)
	{
		++count;
		offset += value.size();
	}
	return count;
}

std::string read_text(const std::filesystem::path& path)
{
	std::ifstream input(path, std::ios::binary);
	return std::string(
		std::istreambuf_iterator<char>(input),
		std::istreambuf_iterator<char>());
}

bool all_digits(const std::string& text, size_t start, size_t count)
{
	for (size_t index = start; index < start + count; ++index)
	{
		if (!std::isdigit(static_cast<unsigned char>(text[index])))
		{
			return false;
		}
	}
	return true;
}

bool has_wall_clock_prefix(const std::string& text)
{
	constexpr size_t kWallClockLength = 25;
	if (text.size() < kWallClockLength)
	{
		return false;
	}
	return text[0] == '[' && text[5] == '-' && text[8] == '-' &&
		text[11] == ' ' && text[14] == ':' && text[17] == ':' &&
		text[20] == '.' && text[24] == ']' &&
		all_digits(text, 1, 4) && all_digits(text, 6, 2) &&
		all_digits(text, 9, 2) && all_digits(text, 12, 2) &&
		all_digits(text, 15, 2) && all_digits(text, 18, 2) &&
		all_digits(text, 21, 3);
}

void prepare_rotation_files(const TestFiles::TemporaryDirectory& root)
{
	const std::filesystem::path log_directory = root.path() / "log";
	std::error_code error;
	std::filesystem::create_directory(log_directory, error);
	TestFiles::write_text(log_directory / "fck1c_efm.log", "previous active\n");
	TestFiles::write_text(log_directory / "fck1c_efm.log.old", "obsolete old\n");
}

void test_rotation_format_and_external_read(Tests::Context& context)
{
	TestFiles::TemporaryDirectory root;
	TEST_EXPECT(context, root.valid());
	prepare_rotation_files(root);
	const std::filesystem::path active = root.path() / "log" / "fck1c_efm.log";
	const std::filesystem::path old = root.path() / "log" / "fck1c_efm.log.old";
	{
		DcsBridge::Internal::EventLog log(root.path().string().c_str());
		TEST_EXPECT(context, log.is_open());
		TEST_EXPECT(context, read_text(old) == "previous active\n");
		TEST_EXPECT(context, log.write({
			DcsBridge::Internal::EventLevel::Info,
			12.5,
			"first event" }));
		TEST_EXPECT(context, log.write({
			DcsBridge::Internal::EventLevel::Error,
			std::nullopt,
			"second event" }));
		const std::string content = TestFiles::read_text_while_open(active);
		TEST_EXPECT(context, has_wall_clock_prefix(content));
		TEST_EXPECT(context, content.find("][12.5][INFO] first event\n") != std::string::npos);
		TEST_EXPECT(context, content.find("][-][ERROR] second event\n") != std::string::npos);
	}
	TEST_EXPECT(context, std::filesystem::is_regular_file(active));
}

void write_concurrent_events(
	DcsBridge::Internal::EventLog& log,
	const char* message,
	std::atomic<int>& successes)
{
	for (int index = 0; index < kWritesPerThread; ++index)
	{
		if (log.write({ DcsBridge::Internal::EventLevel::Info, 1.0, message }))
		{
			++successes;
		}
	}
}

void verify_complete_concurrent_lines(
	Tests::Context& context,
	const std::string& content)
{
	std::istringstream lines(content);
	std::string line;
	int line_count = 0;
	while (std::getline(lines, line))
	{
		++line_count;
		const bool known_line =
			line.find("][1][INFO] worker=a") != std::string::npos ||
			line.find("][1][INFO] worker=b") != std::string::npos;
		TEST_EXPECT(context, known_line);
	}
	TEST_EXPECT(context, line_count == kWritesPerThread * kWriterThreadCount);
}

void test_concurrent_writes_are_complete(Tests::Context& context)
{
	TestFiles::TemporaryDirectory root;
	DcsBridge::Internal::EventLog log(root.path().string().c_str());
	std::atomic<int> successes = 0;
	std::thread first(write_concurrent_events, std::ref(log), "worker=a", std::ref(successes));
	std::thread second(write_concurrent_events, std::ref(log), "worker=b", std::ref(successes));
	first.join();
	second.join();
	TEST_EXPECT(context, successes == kWritesPerThread * kWriterThreadCount);
	const std::filesystem::path active = root.path() / "log" / "fck1c_efm.log";
	verify_complete_concurrent_lines(context, TestFiles::read_text_while_open(active));
}

void test_open_failure_is_exposed(Tests::Context& context)
{
	TestFiles::TemporaryDirectory root;
	const std::filesystem::path invalid_root = root.path() / "missing" / "child";
	DcsBridge::Internal::EventLog log(invalid_root.string().c_str());
	TEST_EXPECT(context, !log.is_open());
	TEST_EXPECT(context, log.last_error_code() != 0);
	TEST_EXPECT(context, !log.write({
		DcsBridge::Internal::EventLevel::Error,
		std::nullopt,
		"cannot be written" }));
	DcsBridge::Internal::EventLog unresolved_path_log("");
	TEST_EXPECT(context, !unresolved_path_log.is_open());
	TEST_EXPECT(context, unresolved_path_log.last_error_code() != 0);
}

void test_error_messages_include_required_parameters(Tests::Context& context)
{
	char message[256];
	Diagnostics::format_callback_lifecycle_error(
		{ message, sizeof(message) },
		{ "ed_fm_get_param", "released", "index", 42 });
	TEST_EXPECT(context, std::string(message) ==
		"callback=ed_fm_get_param index=42 invalid lifecycle state=released");
	Diagnostics::format_invalid_frame_dt_error(
		{ message, sizeof(message) },
		{ -0.25 });
	TEST_EXPECT(context, std::string(message).find("dt=-0.25") != std::string::npos);
	Diagnostics::format_suspension_feedback_error(
		{ message, sizeof(message) },
		{ 7, true });
	TEST_EXPECT(context, std::string(message).find("index=7 info_null=true") !=
		std::string::npos);
}

void test_reporter_writes_required_error_context(Tests::Context& context)
{
	TestFiles::TemporaryDirectory root;
	DcsBridge::Internal::EventLog log(root.path().string().c_str());
	DcsBridge::Internal::OutputStore output_store;
	DcsBridge::Internal::EfmEventReporter reporter(log, output_store);
	reporter.log_unavailable_output({ "ed_fm_get_param", "index", 42 });
	Core::FrameOutput output;
	output.simulation_time_s = 8.25;
	output_store.publish(output);
	reporter.log_invalid_frame_dt(-0.25);
	reporter.log_suspension_feedback_error(7, true);
	output_store.mark_released();
	reporter.log_unavailable_output({ "ed_fm_get_shake_amplitude" });
	const std::filesystem::path active = root.path() / "log" / "fck1c_efm.log";
	const std::string content = TestFiles::read_text_while_open(active);
	TEST_EXPECT(context, content.find(
		"][-][ERROR] callback=ed_fm_get_param index=42 invalid lifecycle state=before_start\n") !=
		std::string::npos);
	TEST_EXPECT(context, content.find(
		"][8.25][ERROR] callback=ed_fm_simulate invalid dt=-0.25\n") !=
		std::string::npos);
	TEST_EXPECT(context, content.find(
		"][8.25][ERROR] callback=ed_fm_suspension_feedback index=7 info_null=true\n") !=
		std::string::npos);
	TEST_EXPECT(context, content.find(
		"][-][ERROR] callback=ed_fm_get_shake_amplitude invalid lifecycle state=released\n") !=
		std::string::npos);
}

void test_reporter_writes_boundary_and_recovery_events(Tests::Context& context)
{
	TestFiles::TemporaryDirectory root;
	DcsBridge::Internal::EventLog log(root.path().string().c_str());
	DcsBridge::Internal::OutputStore output_store;
	DcsBridge::Internal::EfmEventReporter reporter(log, output_store);
	reporter.log_unknown_command(kUnknownCommandId, kUnknownCommandValue);
	reporter.log_missing_param(kMissingParamId);
	reporter.log_missing_param_data(kMissingParamId, "atmosphere");
	DcsBridge::Internal::CockpitParameterEvents events;
	events.items[0] = {
		DcsBridge::Internal::CockpitParameterEventType::Error,
		"TEST_PARAM", "missing_handle" };
	events.items[1] = {
		DcsBridge::Internal::CockpitParameterEventType::Recovery,
		"TEST_PARAM" };
	events.count = 2;
	reporter.log_cockpit_parameter_events(events);
	const std::string content = TestFiles::read_text_while_open(
		root.path() / "log" / "fck1c_efm.log");
	const std::string unknown_command = "callback=ed_fm_set_command command=" +
		std::to_string(kUnknownCommandId) + " unknown value=0.5";
	const std::string missing_param = "callback=ed_fm_get_param index=" +
		std::to_string(kMissingParamId) + " missing mapping";
	const std::string missing_data = "callback=ed_fm_get_param index=" +
		std::to_string(kMissingParamId) + " missing data=atmosphere";
	TEST_EXPECT(context, content.find(unknown_command) != std::string::npos);
	TEST_EXPECT(context, content.find("][WARN] " + unknown_command) !=
		std::string::npos);
	TEST_EXPECT(context, content.find(missing_param) != std::string::npos);
	TEST_EXPECT(context, content.find("][WARN] " + missing_param) !=
		std::string::npos);
	TEST_EXPECT(context, content.find(missing_data) != std::string::npos);
	TEST_EXPECT(context, content.find("][ERROR] " + missing_data) !=
		std::string::npos);
	TEST_EXPECT(context, content.find(
		"cockpit parameter=TEST_PARAM unavailable reason=missing_handle") !=
		std::string::npos);
	TEST_EXPECT(context, content.find(
		"][INFO] cockpit parameter=TEST_PARAM recovered") != std::string::npos);
}

void test_reporter_writes_repeated_start_warning(Tests::Context& context)
{
	TestFiles::TemporaryDirectory root;
	DcsBridge::Internal::EventLog log(root.path().string().c_str());
	DcsBridge::Internal::OutputStore output_store;
	DcsBridge::Internal::EfmEventReporter reporter(log, output_store);
	Core::FrameOutput output;
	output.simulation_time_s = 12.5;
	output_store.publish_start(output);
	reporter.log_repeated_start(Core::StartMode::HotAir);
	const std::string content = TestFiles::read_text_while_open(
		root.path() / "log" / "fck1c_efm.log");
	TEST_EXPECT(context, content.find(
		"][12.5][WARN] flight start "
		"lifecycle_warning=repeated_start_without_release "
		"mode=hot_air action=replace_without_implicit_release\n") !=
		std::string::npos);
}

void test_counted_warnings_summarize_and_reset(Tests::Context& context)
{
	constexpr int kObservedUnknownCommand = 2659;
	TestFiles::TemporaryDirectory root;
	DcsBridge::Internal::EventLog log(root.path().string().c_str());
	DcsBridge::Internal::OutputStore output_store;
	DcsBridge::Internal::EfmEventReporter reporter(log, output_store);
	Core::FrameOutput output;
	output.simulation_time_s = 4.0;
	output_store.publish(output);
	reporter.log_unknown_command(kObservedUnknownCommand, 0.25F);
	reporter.log_unknown_command(kObservedUnknownCommand, 0.5F);
	reporter.log_missing_param(kObservedUnknownCommand);
	reporter.log_missing_param(kObservedUnknownCommand);
	reporter.log_release(4.0);
	output.simulation_time_s = 8.0;
	output_store.publish(output);
	reporter.log_unknown_command(kObservedUnknownCommand, 0.75F);
	reporter.log_missing_param(kObservedUnknownCommand);
	reporter.log_release(8.0);
	const std::string content = TestFiles::read_text_while_open(
		root.path() / "log" / "fck1c_efm.log");
	const std::string command_event =
		"callback=ed_fm_set_command command=2659 unknown";
	TEST_EXPECT(context, count_occurrences(content, command_event) == 2);
	TEST_EXPECT(context, content.find("command=2659 unknown value=0.5") ==
		std::string::npos);
	TEST_EXPECT(context, count_occurrences(
		content,
		"kind=unknown_command id=2659 total=2 flight_release_summary") == 1);
	TEST_EXPECT(context, count_occurrences(
		content,
		"kind=unknown_command id=2659 total=1 flight_release_summary") == 1);
	TEST_EXPECT(context, count_occurrences(
		content,
		"kind=unknown_param id=2659 total=2 flight_release_summary") == 1);
	TEST_EXPECT(context, count_occurrences(
		content,
		"kind=unknown_param id=2659 total=1 flight_release_summary") == 1);
	TEST_EXPECT(context, count_occurrences(
		content,
		"callback=ed_fm_get_param index=2659 missing mapping") == 2);
}
}

void run_event_log_tests(Tests::Context& context)
{
	test_rotation_format_and_external_read(context);
	test_concurrent_writes_are_complete(context);
	test_open_failure_is_exposed(context);
	test_error_messages_include_required_parameters(context);
	test_reporter_writes_required_error_context(context);
	test_reporter_writes_boundary_and_recovery_events(context);
	test_reporter_writes_repeated_start_warning(context);
	test_counted_warnings_summarize_and_reset(context);
}
